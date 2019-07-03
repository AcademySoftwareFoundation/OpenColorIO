#include <string>
#include <vector>
#include <set>
#include <fstream>
#include <iostream>
#include <sstream>
#include <memory>
#include <regex>
#include <exception>
#include <ciso646>


extern "C" {
#if defined(_WIN32) || defined(_WIN64) || defined(_WINDOWS) || defined(_MSC_VER)
#include <direct.h>
#define GETCWD _getcwd
#else
#include <unistd.h>
#define GETCWD getcwd
#endif
}


struct Hunk {
	int start_src;
	int lines_src;
	int start_target;
	int lines_target;
	std::vector<std::string> text;
};

struct Patch {
	enum class DiffType {UNKNOWN, DIFF};
	std::string source;
	std::string target;
	std::vector<Hunk> hunks;
	std::string src_file;
	std::size_t src_line;
	DiffType type;
};

using PatchPtr = std::unique_ptr<Patch>;
using HunkPtr = std::unique_ptr<Hunk>;

const std::regex re_header_start("^--- (\\S+)\\s+(.+)$");
const std::regex re_header_end("^\\+\\+\\+ (\\S+)\\s+(.+)$");
const std::regex re_hunk_start("^\\@\\@ -(\\d+),(\\d+) \\+(\\d+),(\\d+) \\@\\@$");


std::string get_current_working_dir() {
	char buff[FILENAME_MAX];
	GETCWD(buff, FILENAME_MAX);
	return std::string(buff);
}

std::string windowsify_filepath(const std::string& filepath) {
	std::string fixed = filepath;
	std::regex unix_slashes("/");
	fixed = std::regex_replace(fixed, unix_slashes, "\\");
	return fixed;
}
std::string unixify_filepath(const std::string& filepath) {
	std::string fixed = filepath;
	std::regex windows_slashes("\\\\");
	fixed = std::regex_replace(fixed, windows_slashes, "/");
	return fixed;
}

#if defined(_WIN32) || defined(_WIN64) || defined(_WINDOWS) || defined(_MSC_VER)
#define fix_filepath windowsify_filepath
#define path_seperator "\\"
#define path_regex "\\\\"
#else
#define fix_filepath unixify_filepath
#define path_seperator "/"
#define path_regex "/"
#endif


std::vector<std::string> filepath_split(const std::string& filepath) {
	std::vector<std::string> elements;
	std::regex rgx(path_regex);
	std::sregex_token_iterator iter(filepath.begin(), filepath.end(), rgx, -1);
	std::sregex_token_iterator end;
	while (iter != end) {
		elements.push_back(*iter);
		++iter;
	}
	return elements;
}

std::string filepath_join(const std::vector<std::string>& elements) {
	std::string joined;
	if (not elements.empty()) {
		joined = elements.front();
	}
	for (std::size_t index = 1; index < elements.size(); ++index) {
		joined += std::string(path_seperator) + elements[index];
	}
	return joined;
}

std::vector<Patch> read_patch_file(std::istream& input_stream, const std::string& filename) {
	std::vector<Patch> patches;
	PatchPtr current_patch;
	HunkPtr current_hunk;
	Patch::DiffType type = Patch::DiffType::UNKNOWN;

	std::size_t line_num = 1;
	for (std::string line; std::getline(input_stream, line, '\n'); ++line_num) {
		std::smatch match;
		if (std::regex_match(line, match, re_header_start)) {
			if (current_patch) {
				//TODO: check patch is complete / valid
				current_patch->hunks.push_back(std::move(*current_hunk.release()));
				patches.push_back(std::move(*current_patch.release()));
			}
			current_patch = PatchPtr(new Patch);
			current_patch->source = match[1];
			current_patch->src_file = filename;
			current_patch->src_line = line_num;
			current_patch->type = type;
		} 
		else if (std::regex_match(line, match, re_header_end)) {
			if (not current_patch or not current_patch->target.empty()) {
				std::stringstream ss;
				ss << "Error parsing line " << line_num << " of patch file '" << filename << "'" << std::endl;
				if (!current_patch){ ss << "\tMissing --- header declaration" << std::endl; }
				else { ss << "\tDuplicate +++ header declaration" << std::endl; }
				throw std::runtime_error(ss.str().c_str());
			}
			current_patch->target = match[1];
		}
		else if (std::regex_match(line, match, re_hunk_start)) {
			if (current_hunk) {
				if (not current_patch) {
					std::stringstream ss;
					ss << "Error parsing line " << line_num << " of patch file '" << filename << "'" << std::endl;
					ss << "\tHunk found without prior --- header declaration" << std::endl;
					throw std::runtime_error(ss.str().c_str());
				}
				current_patch->hunks.push_back(std::move(*current_hunk.release()));
			}
			current_hunk = HunkPtr(new Hunk);
			current_hunk->start_src = std::stoi(match[1]);
			current_hunk->lines_src = std::stoi(match[2]);
			current_hunk->start_target = std::stoi(match[3]);
			current_hunk->lines_target = std::stoi(match[4]);
		}
		else {
			if (line.substr(0, 4) == "diff") {
				// This line is a diff header
				type = Patch::DiffType::DIFF;
				continue;
			}
			if (not current_hunk) {
				std::stringstream ss;
				ss << "Error parsing line " << line_num << " of patch file '" << filename << "'" << std::endl;
				ss << "\tExpected @@ hunk declaration before hunk body" << std::endl;
				throw std::runtime_error(ss.str().c_str());
			}
			current_hunk->text.push_back(line);
		}
	}
	current_patch->hunks.push_back(std::move(*current_hunk.release()));
	patches.push_back(std::move(*current_patch.release()));
	return patches;
}

void apply_patch(const Patch& patch, std::istream& src, std::ostream& dst) {
	int src_line_num = 1;
	std::string line;
	std::getline(src, line, '\n');
	for (const auto &hunk : patch.hunks) {
		while (src_line_num < hunk.start_src) {
			dst << line << std::endl;
			std::getline(src, line, '\n');
			++src_line_num;
		}
		for (const auto& hline : hunk.text) {
			if (hline.substr(0, 1) == "-" or hline.substr(0, 1) == "\\") {
				std::getline(src, line, '\n');
				++src_line_num;
				continue;
			}
			else {
				if (not(hline.substr(0, 1) == "+")) {
					std::getline(src, line, '\n');
					++src_line_num;
				}
				dst << hline.substr(1) << std::endl;
			}
		}
	}
	dst << line << std::endl;
	for (; std::getline(src, line, '\n');)
	{
		dst << line << std::endl;
	}
}


int main(int argc, const char* argv[]){
	try {
		std::string patch_file;
		std::size_t strip = 0;

		int i = 1;
		while (i < argc) {
			using str_set = std::set < std::string > ;
			std::string arg(argv[i]);
			if (str_set{"-p", "--strip"}.count(arg)) {
				if (i == argc) {
					std::stringstream ss;
					ss << "Missing required argument to " << arg << "option" << std::endl;
					throw std::runtime_error(ss.str().c_str());
				}
				std::string value(argv[++i]);
				std::stringstream ss;
				ss << value;
				ss >> strip;
			}
			else if (str_set{ "-f", "--force" }.count(arg)) {
				// not supported, but silently swallow the arg
			}
			else if (str_set{ "-i", "--input" }.count(arg)) {
				if (i == argc) {
					std::stringstream ss;
					ss << "Missing required argument to " << arg << "option" << std::endl;
					throw std::runtime_error(ss.str().c_str());
				}
				std::string value(argv[++i]);
				patch_file = value;
			}
			else {
					std::stringstream ss;
					ss << "Unknown argument" << arg << std::endl;
					throw std::runtime_error(ss.str().c_str());
			}
			++i;
		}

		std::istream* input_stream = nullptr;
		std::fstream patch_file_stream;
		if (patch_file.empty()) {
			input_stream = &std::cin;
			patch_file = "stdin";
		}
		else {
			patch_file_stream = std::fstream(patch_file, std::ios::binary | std::ios::in);
			if (not patch_file_stream) {
				std::stringstream ss;
				ss << "Unable to open patch file '" << patch_file << "'" << std::endl;
				throw std::runtime_error(ss.str().c_str());
			}
			input_stream = &patch_file_stream;
		}

		auto patches = read_patch_file(*input_stream, patch_file);

		std::string cwd = fix_filepath(get_current_working_dir());

		for (const auto& patch : patches) {
			// try to find file to patch
			std::string source = fix_filepath(patch.source);
			auto elems = filepath_split(source);
			for (std::size_t s = strip; s != 0; --s) {
				elems.erase(elems.begin());
			}
			std::string stripped = filepath_join(elems);
			std::string backup_filepath = stripped + ".orig";

			// Make a backup of our original file
			std::fstream src(stripped, std::ios::binary | std::ios::in);
			if (src) {
				// make a copy of the original file
				std::fstream backup(backup_filepath, std::ios::binary | std::ios::trunc | std::ios::out);
				if (not backup) {
					std::cerr << backup_filepath << std::endl;
					throw std::runtime_error("could not create temp backup");
				}
				backup << src.rdbuf();
				src.close();
				src = std::fstream(backup_filepath, std::ios::binary | std::ios::in);
				if (not src) {
						std::cerr << backup_filepath << std::endl;
						throw std::runtime_error("could not read backup file");
				}
			}
			else {
				if (patch.type == Patch::DiffType::DIFF) {
					// Non existing inputs are not an error for DIFF types. They are new files to be made
					src = std::fstream();
				}
				else {
					std::stringstream ss;
					ss << "Cannot find file '" << stripped << "' required by patch '" << patch.src_file << "' at line " << patch.src_line << std::endl;
					ss << "Perhaps you should have used the -p or --strip option?" << std::endl;
					throw std::runtime_error(ss.str().c_str());
				}
			}

			std::fstream dst = std::fstream(stripped, std::ios::binary | std::ios::trunc | std::ios::out);
			if (not dst) {
				std::stringstream ss;
				ss << "Failed to create new output file '" << stripped << "'" << std::endl;
				throw std::runtime_error(ss.str().c_str());
			}

			src.seekg(0);
			dst.seekp(0);
			try {
				apply_patch(patch, src, dst);
				std::cout << "Successfully patched '" << stripped << "'" << std::endl;
			}
			catch (std::exception& e) {
				src.seekg(0);
				dst.seekp(0);
				dst << src.rdbuf();
				std::remove(backup_filepath.c_str());
				throw e;
			}
			std::remove(backup_filepath.c_str());
		}
	}
	catch (std::runtime_error & e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
}
