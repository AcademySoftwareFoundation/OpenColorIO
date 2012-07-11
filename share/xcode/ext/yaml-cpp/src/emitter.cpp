#include "yaml-cpp/emitter.h"
#include "emitterstate.h"
#include "emitterutils.h"
#include "indentation.h"
#include "yaml-cpp/exceptions.h"
#include <sstream>

namespace YAML
{	
	Emitter::Emitter(): m_pState(new EmitterState)
	{
	}
	
	Emitter::~Emitter()
	{
	}
	
	const char *Emitter::c_str() const
	{
		return m_stream.str();
	}
	
	unsigned Emitter::size() const
	{
		return m_stream.pos();
	}
	
	// state checking
	bool Emitter::good() const
	{
		return m_pState->good();
	}
	
	const std::string Emitter::GetLastError() const
	{
		return m_pState->GetLastError();
	}

	// global setters
	bool Emitter::SetOutputCharset(EMITTER_MANIP value)
	{
		return m_pState->SetOutputCharset(value, GLOBAL);
	}

	bool Emitter::SetStringFormat(EMITTER_MANIP value)
	{
		return m_pState->SetStringFormat(value, GLOBAL);
	}
	
	bool Emitter::SetBoolFormat(EMITTER_MANIP value)
	{
		bool ok = false;
		if(m_pState->SetBoolFormat(value, GLOBAL))
			ok = true;
		if(m_pState->SetBoolCaseFormat(value, GLOBAL))
			ok = true;
		if(m_pState->SetBoolLengthFormat(value, GLOBAL))
			ok = true;
		return ok;
	}
	
	bool Emitter::SetIntBase(EMITTER_MANIP value)
	{
		return m_pState->SetIntFormat(value, GLOBAL);
	}
	
	bool Emitter::SetSeqFormat(EMITTER_MANIP value)
	{
		return m_pState->SetFlowType(GT_SEQ, value, GLOBAL);
	}
	
	bool Emitter::SetMapFormat(EMITTER_MANIP value)
	{
		bool ok = false;
		if(m_pState->SetFlowType(GT_MAP, value, GLOBAL))
			ok = true;
		if(m_pState->SetMapKeyFormat(value, GLOBAL))
			ok = true;
		return ok;
	}
	
	bool Emitter::SetIndent(unsigned n)
	{
		return m_pState->SetIndent(n, GLOBAL);
	}
	
	bool Emitter::SetPreCommentIndent(unsigned n)
	{
		return m_pState->SetPreCommentIndent(n, GLOBAL);
	}
	
	bool Emitter::SetPostCommentIndent(unsigned n)
	{
		return m_pState->SetPostCommentIndent(n, GLOBAL);
	}
    
    bool Emitter::SetFloatPrecision(unsigned n)
    {
        return m_pState->SetFloatPrecision(n, GLOBAL);
    }

    bool Emitter::SetDoublePrecision(unsigned n)
    {
        return m_pState->SetDoublePrecision(n, GLOBAL);
    }

	// SetLocalValue
	// . Either start/end a group, or set a modifier locally
	Emitter& Emitter::SetLocalValue(EMITTER_MANIP value)
	{
		if(!good())
			return *this;
		
		switch(value) {
			case BeginDoc:
				EmitBeginDoc();
				break;
			case EndDoc:
				EmitEndDoc();
				break;
			case BeginSeq:
				EmitBeginSeq();
				break;
			case EndSeq:
				EmitEndSeq();
				break;
			case BeginMap:
				EmitBeginMap();
				break;
			case EndMap:
				EmitEndMap();
				break;
			case Key:
				EmitKey();
				break;
			case Value:
				EmitValue();
				break;
			case TagByKind:
				EmitKindTag();
				break;
			case Newline:
				EmitNewline();
				break;
			default:
				m_pState->SetLocalValue(value);
				break;
		}
		return *this;
	}
	
	Emitter& Emitter::SetLocalIndent(const _Indent& indent)
	{
		m_pState->SetIndent(indent.value, LOCAL);
		return *this;
	}

    Emitter& Emitter::SetLocalPrecision(const _Precision& precision)
    {
        if(precision.floatPrecision >= 0)
            m_pState->SetFloatPrecision(precision.floatPrecision, LOCAL);
        if(precision.doublePrecision >= 0)
            m_pState->SetDoublePrecision(precision.doublePrecision, LOCAL);
        return *this;
    }

	// GotoNextPreAtomicState
	// . Runs the state machine, emitting if necessary, and returns 'true' if done (i.e., ready to emit an atom)
	bool Emitter::GotoNextPreAtomicState()
	{
		if(!good())
			return true;
		
		unsigned curIndent = m_pState->GetCurIndent();
		
		EMITTER_STATE curState = m_pState->GetCurState();
		switch(curState) {
				// document-level
			case ES_WAITING_FOR_DOC:
				m_pState->SwitchState(ES_WRITING_DOC);
				return true;
			case ES_WRITING_DOC:
				return true;
			case ES_DONE_WITH_DOC:
				EmitBeginDoc();
				return false;
				
				// block sequence
			case ES_WAITING_FOR_BLOCK_SEQ_ENTRY:
				m_stream << IndentTo(curIndent) << "-";
				m_pState->RequireSoftSeparation();
				m_pState->SwitchState(ES_WRITING_BLOCK_SEQ_ENTRY);
				return true;
			case ES_WRITING_BLOCK_SEQ_ENTRY:
				return true;
			case ES_DONE_WITH_BLOCK_SEQ_ENTRY:
				m_stream << '\n';
				m_pState->SwitchState(ES_WAITING_FOR_BLOCK_SEQ_ENTRY);
				return false;
				
				// flow sequence
			case ES_WAITING_FOR_FLOW_SEQ_ENTRY:
				m_pState->SwitchState(ES_WRITING_FLOW_SEQ_ENTRY);
				return true;
			case ES_WRITING_FLOW_SEQ_ENTRY:
				return true;
			case ES_DONE_WITH_FLOW_SEQ_ENTRY:
				EmitSeparationIfNecessary();
				m_stream << ',';
				m_pState->RequireSoftSeparation();
				m_pState->SwitchState(ES_WAITING_FOR_FLOW_SEQ_ENTRY);
				return false;
				
				// block map
			case ES_WAITING_FOR_BLOCK_MAP_ENTRY:
				m_pState->SetError(ErrorMsg::EXPECTED_KEY_TOKEN);
				return true;
			case ES_WAITING_FOR_BLOCK_MAP_KEY:
				if(m_pState->CurrentlyInLongKey()) {
					m_stream << IndentTo(curIndent) << '?';
					m_pState->RequireSoftSeparation();
				}
				m_pState->SwitchState(ES_WRITING_BLOCK_MAP_KEY);
				return true;
			case ES_WRITING_BLOCK_MAP_KEY:
				return true;
			case ES_DONE_WITH_BLOCK_MAP_KEY:
				m_pState->SetError(ErrorMsg::EXPECTED_VALUE_TOKEN);
				return true;
			case ES_WAITING_FOR_BLOCK_MAP_VALUE:
				m_pState->SwitchState(ES_WRITING_BLOCK_MAP_VALUE);
				return true;
			case ES_WRITING_BLOCK_MAP_VALUE:
				return true;
			case ES_DONE_WITH_BLOCK_MAP_VALUE:
				m_pState->SetError(ErrorMsg::EXPECTED_KEY_TOKEN);
				return true;
				
				// flow map
			case ES_WAITING_FOR_FLOW_MAP_ENTRY:
				m_pState->SetError(ErrorMsg::EXPECTED_KEY_TOKEN);
				return true;
			case ES_WAITING_FOR_FLOW_MAP_KEY:
				EmitSeparationIfNecessary();
				m_pState->SwitchState(ES_WRITING_FLOW_MAP_KEY);
				if(m_pState->CurrentlyInLongKey()) {
					m_stream << '?';
					m_pState->RequireSoftSeparation();
				}
				return true;
			case ES_WRITING_FLOW_MAP_KEY:
				return true;
			case ES_DONE_WITH_FLOW_MAP_KEY:
				m_pState->SetError(ErrorMsg::EXPECTED_VALUE_TOKEN);
				return true;
			case ES_WAITING_FOR_FLOW_MAP_VALUE:
				EmitSeparationIfNecessary();
				m_stream << ':';
				m_pState->RequireSoftSeparation();
				m_pState->SwitchState(ES_WRITING_FLOW_MAP_VALUE);
				return true;
			case ES_WRITING_FLOW_MAP_VALUE:
				return true;
			case ES_DONE_WITH_FLOW_MAP_VALUE:
				m_pState->SetError(ErrorMsg::EXPECTED_KEY_TOKEN);
				return true;
			default:
				assert(false);
		}

		assert(false);
		return true;
	}
	
	// PreAtomicWrite
	// . Depending on the emitter state, write to the stream to get it
	//   in position to do an atomic write (e.g., scalar, sequence, or map)
	void Emitter::PreAtomicWrite()
	{
		if(!good())
			return;
		
		while(!GotoNextPreAtomicState())
			;
	}
	
	// PostAtomicWrite
	// . Clean up
	void Emitter::PostAtomicWrite()
	{
		if(!good())
			return;
		
		EMITTER_STATE curState = m_pState->GetCurState();
		switch(curState) {
				// document-level
			case ES_WRITING_DOC:
				m_pState->SwitchState(ES_DONE_WITH_DOC);
				break;

				// block seq
			case ES_WRITING_BLOCK_SEQ_ENTRY:
				m_pState->SwitchState(ES_DONE_WITH_BLOCK_SEQ_ENTRY);
				break;
				
				// flow seq
			case ES_WRITING_FLOW_SEQ_ENTRY:
				m_pState->SwitchState(ES_DONE_WITH_FLOW_SEQ_ENTRY);
				break;
				
				// block map
			case ES_WRITING_BLOCK_MAP_KEY:
				if(!m_pState->CurrentlyInLongKey()) {
					m_stream << ':';
					m_pState->RequireSoftSeparation();
				}
				m_pState->SwitchState(ES_DONE_WITH_BLOCK_MAP_KEY);
				break;
			case ES_WRITING_BLOCK_MAP_VALUE:
				m_pState->SwitchState(ES_DONE_WITH_BLOCK_MAP_VALUE);
				break;
				
				// flow map
			case ES_WRITING_FLOW_MAP_KEY:
				m_pState->SwitchState(ES_DONE_WITH_FLOW_MAP_KEY);
				break;
			case ES_WRITING_FLOW_MAP_VALUE:
				m_pState->SwitchState(ES_DONE_WITH_FLOW_MAP_VALUE);
				break;
			default:
				assert(false);
		};
				
		m_pState->ClearModifiedSettings();
	}
	
	// EmitSeparationIfNecessary
	void Emitter::EmitSeparationIfNecessary()
	{
		if(!good())
			return;
		
		if(m_pState->RequiresSoftSeparation())
			m_stream << ' ';
		else if(m_pState->RequiresHardSeparation())
			m_stream << '\n';
		m_pState->UnsetSeparation();
	}
	
	// EmitBeginDoc
	void Emitter::EmitBeginDoc()
	{
		if(!good())
			return;
		
		EMITTER_STATE curState = m_pState->GetCurState();
		if(curState != ES_WAITING_FOR_DOC && curState != ES_WRITING_DOC && curState != ES_DONE_WITH_DOC) {
			m_pState->SetError("Unexpected begin document");
			return;
		}
		
		if(curState == ES_WRITING_DOC || curState == ES_DONE_WITH_DOC)
			m_stream << '\n';		
		m_stream << "---\n";

		m_pState->UnsetSeparation();
		m_pState->SwitchState(ES_WAITING_FOR_DOC);
	}
	
	// EmitEndDoc
	void Emitter::EmitEndDoc()
	{
		if(!good())
			return;

		
		EMITTER_STATE curState = m_pState->GetCurState();
		if(curState != ES_WAITING_FOR_DOC && curState != ES_WRITING_DOC && curState != ES_DONE_WITH_DOC) {
			m_pState->SetError("Unexpected end document");
			return;
		}
		
		if(curState == ES_WRITING_DOC || curState == ES_DONE_WITH_DOC)
			m_stream << '\n';		
		m_stream << "...\n";
		
		m_pState->UnsetSeparation();
		m_pState->SwitchState(ES_WAITING_FOR_DOC);
	}

	// EmitBeginSeq
	void Emitter::EmitBeginSeq()
	{
		if(!good())
			return;
		
		// must have a long key if we're emitting a sequence
		m_pState->StartLongKey();
		
		PreAtomicWrite();
		
		EMITTER_STATE curState = m_pState->GetCurState();
		EMITTER_MANIP flowType = m_pState->GetFlowType(GT_SEQ);
		if(flowType == Block) {
			if(curState == ES_WRITING_BLOCK_SEQ_ENTRY ||
			   curState == ES_WRITING_BLOCK_MAP_KEY || curState == ES_WRITING_BLOCK_MAP_VALUE ||
			   curState == ES_WRITING_DOC
			) {
				if(m_pState->RequiresHardSeparation() || curState != ES_WRITING_DOC) {
					m_stream << "\n";
					m_pState->UnsetSeparation();
				}
			}
			m_pState->PushState(ES_WAITING_FOR_BLOCK_SEQ_ENTRY);
		} else if(flowType == Flow) {
			EmitSeparationIfNecessary();
			m_stream << "[";
			m_pState->PushState(ES_WAITING_FOR_FLOW_SEQ_ENTRY);
		} else
			assert(false);

		m_pState->BeginGroup(GT_SEQ);
	}
	
	// EmitEndSeq
	void Emitter::EmitEndSeq()
	{
		if(!good())
			return;
		
		if(m_pState->GetCurGroupType() != GT_SEQ)
			return m_pState->SetError(ErrorMsg::UNEXPECTED_END_SEQ);
		
		EMITTER_STATE curState = m_pState->GetCurState();
		FLOW_TYPE flowType = m_pState->GetCurGroupFlowType();
		if(flowType == FT_BLOCK) {
			// Note: block sequences are *not* allowed to be empty, but we convert it
			//       to a flow sequence if it is
			assert(curState == ES_DONE_WITH_BLOCK_SEQ_ENTRY || curState == ES_WAITING_FOR_BLOCK_SEQ_ENTRY);
			if(curState == ES_WAITING_FOR_BLOCK_SEQ_ENTRY) {
				// Note: only one of these will actually output anything for a given situation
				EmitSeparationIfNecessary();
				unsigned curIndent = m_pState->GetCurIndent();
				m_stream << IndentTo(curIndent);

				m_stream << "[]";
			}
		} else if(flowType == FT_FLOW) {
			// Note: flow sequences are allowed to be empty
			assert(curState == ES_DONE_WITH_FLOW_SEQ_ENTRY || curState == ES_WAITING_FOR_FLOW_SEQ_ENTRY);
			m_stream << "]";
		} else
			assert(false);
		
		m_pState->PopState();
		m_pState->EndGroup(GT_SEQ);

		PostAtomicWrite();
	}
	
	// EmitBeginMap
	void Emitter::EmitBeginMap()
	{
		if(!good())
			return;
		
		// must have a long key if we're emitting a map
		m_pState->StartLongKey();

		PreAtomicWrite();

		EMITTER_STATE curState = m_pState->GetCurState();
		EMITTER_MANIP flowType = m_pState->GetFlowType(GT_MAP);
		if(flowType == Block) {
			if(curState == ES_WRITING_BLOCK_SEQ_ENTRY ||
			   curState == ES_WRITING_BLOCK_MAP_KEY || curState == ES_WRITING_BLOCK_MAP_VALUE ||
			   curState == ES_WRITING_DOC
			) {
				if(m_pState->RequiresHardSeparation() || (curState != ES_WRITING_DOC && curState != ES_WRITING_BLOCK_SEQ_ENTRY)) {
					m_stream << "\n";
					m_pState->UnsetSeparation();
				}
			}
			m_pState->PushState(ES_WAITING_FOR_BLOCK_MAP_ENTRY);
		} else if(flowType == Flow) {
			EmitSeparationIfNecessary();
			m_stream << "{";
			m_pState->PushState(ES_WAITING_FOR_FLOW_MAP_ENTRY);
		} else
			assert(false);
		
		m_pState->BeginGroup(GT_MAP);
	}
	
	// EmitEndMap
	void Emitter::EmitEndMap()
	{
		if(!good())
			return;
		
		if(m_pState->GetCurGroupType() != GT_MAP)
			return m_pState->SetError(ErrorMsg::UNEXPECTED_END_MAP);

		EMITTER_STATE curState = m_pState->GetCurState();
		FLOW_TYPE flowType = m_pState->GetCurGroupFlowType();
		if(flowType == FT_BLOCK) {
			// Note: block sequences are *not* allowed to be empty, but we convert it
			//       to a flow sequence if it is
			assert(curState == ES_DONE_WITH_BLOCK_MAP_VALUE || curState == ES_WAITING_FOR_BLOCK_MAP_ENTRY);
			if(curState == ES_WAITING_FOR_BLOCK_MAP_ENTRY) {
				// Note: only one of these will actually output anything for a given situation
				EmitSeparationIfNecessary();
				unsigned curIndent = m_pState->GetCurIndent();
				m_stream << IndentTo(curIndent);
				m_stream << "{}";
			}
		} else if(flowType == FT_FLOW) {
			// Note: flow maps are allowed to be empty
			assert(curState == ES_DONE_WITH_FLOW_MAP_VALUE || curState == ES_WAITING_FOR_FLOW_MAP_ENTRY);
			EmitSeparationIfNecessary();
			m_stream << "}";
		} else
			assert(false);
		
		m_pState->PopState();
		m_pState->EndGroup(GT_MAP);
		
		PostAtomicWrite();
	}
	
	// EmitKey
	void Emitter::EmitKey()
	{
		if(!good())
			return;
		
		EMITTER_STATE curState = m_pState->GetCurState();
		FLOW_TYPE flowType = m_pState->GetCurGroupFlowType();
		if(curState != ES_WAITING_FOR_BLOCK_MAP_ENTRY && curState != ES_DONE_WITH_BLOCK_MAP_VALUE
		   && curState != ES_WAITING_FOR_FLOW_MAP_ENTRY && curState != ES_DONE_WITH_FLOW_MAP_VALUE)
			return m_pState->SetError(ErrorMsg::UNEXPECTED_KEY_TOKEN);

		if(flowType == FT_BLOCK) {
			if(curState == ES_DONE_WITH_BLOCK_MAP_VALUE)
				m_stream << '\n';
			unsigned curIndent = m_pState->GetCurIndent();
			m_stream << IndentTo(curIndent);
			m_pState->UnsetSeparation();
			m_pState->SwitchState(ES_WAITING_FOR_BLOCK_MAP_KEY);
		} else if(flowType == FT_FLOW) {
			EmitSeparationIfNecessary();
			if(curState == ES_DONE_WITH_FLOW_MAP_VALUE) {
				m_stream << ',';
				m_pState->RequireSoftSeparation();
			}
			m_pState->SwitchState(ES_WAITING_FOR_FLOW_MAP_KEY);
		} else
			assert(false);
		
		if(m_pState->GetMapKeyFormat() == LongKey)
			m_pState->StartLongKey();
		else if(m_pState->GetMapKeyFormat() == Auto)
			m_pState->StartSimpleKey();
		else
			assert(false);
	}
	
	// EmitValue
	void Emitter::EmitValue()
	{
		if(!good())
			return;

		EMITTER_STATE curState = m_pState->GetCurState();
		FLOW_TYPE flowType = m_pState->GetCurGroupFlowType();
		if(curState != ES_DONE_WITH_BLOCK_MAP_KEY && curState != ES_DONE_WITH_FLOW_MAP_KEY)
			return m_pState->SetError(ErrorMsg::UNEXPECTED_VALUE_TOKEN);

		if(flowType == FT_BLOCK) {
			if(m_pState->CurrentlyInLongKey()) {
				m_stream << '\n';
				m_stream << IndentTo(m_pState->GetCurIndent());
				m_stream << ':';
				m_pState->RequireSoftSeparation();
			}
			m_pState->SwitchState(ES_WAITING_FOR_BLOCK_MAP_VALUE);
		} else if(flowType == FT_FLOW) {
			m_pState->SwitchState(ES_WAITING_FOR_FLOW_MAP_VALUE);
		} else
			assert(false);
	}

	// EmitNewline
	void Emitter::EmitNewline()
	{
		if(!good())
			return;

		if(CanEmitNewline()) {
			m_stream << '\n';
			m_pState->UnsetSeparation();
		}
	}

	bool Emitter::CanEmitNewline() const
	{
		FLOW_TYPE flowType = m_pState->GetCurGroupFlowType();
		if(flowType == FT_BLOCK && m_pState->CurrentlyInLongKey())
			return true;

		EMITTER_STATE curState = m_pState->GetCurState();
		return curState != ES_DONE_WITH_BLOCK_MAP_KEY && curState != ES_WAITING_FOR_BLOCK_MAP_VALUE && curState != ES_WRITING_BLOCK_MAP_VALUE;
	}

	// *******************************************************************************************
	// overloads of Write
	
	Emitter& Emitter::Write(const std::string& str)
	{
		if(!good())
			return *this;
		
		// literal scalars must use long keys
		if(m_pState->GetStringFormat() == Literal && m_pState->GetCurGroupFlowType() != FT_FLOW)
			m_pState->StartLongKey();
		
		PreAtomicWrite();
		EmitSeparationIfNecessary();
		
		bool escapeNonAscii = m_pState->GetOutputCharset() == EscapeNonAscii;
		EMITTER_MANIP strFmt = m_pState->GetStringFormat();
		FLOW_TYPE flowType = m_pState->GetCurGroupFlowType();
		unsigned curIndent = m_pState->GetCurIndent();

		switch(strFmt) {
			case Auto:
				Utils::WriteString(m_stream, str, flowType == FT_FLOW, escapeNonAscii);
				break;
			case SingleQuoted:
				if(!Utils::WriteSingleQuotedString(m_stream, str)) {
					m_pState->SetError(ErrorMsg::SINGLE_QUOTED_CHAR);
					return *this;
				}
				break;
			case DoubleQuoted:
				Utils::WriteDoubleQuotedString(m_stream, str, escapeNonAscii);
				break;
			case Literal:
				if(flowType == FT_FLOW)
					Utils::WriteString(m_stream, str, flowType == FT_FLOW, escapeNonAscii);
				else
					Utils::WriteLiteralString(m_stream, str, curIndent + m_pState->GetIndent());
				break;
			default:
				assert(false);
		}
		
		PostAtomicWrite();
		return *this;
	}

	void Emitter::PreWriteIntegralType(std::stringstream& str)
	{
		PreAtomicWrite();
		EmitSeparationIfNecessary();
		
		EMITTER_MANIP intFmt = m_pState->GetIntFormat();
		switch(intFmt) {
			case Dec:
				str << std::dec;
				break;
			case Hex:
                str << "0x";
				str << std::hex;
				break;
			case Oct:
                str << "0";
				str << std::oct;
				break;
			default:
				assert(false);
		}
	}

	void Emitter::PreWriteStreamable(std::stringstream&)
	{
		PreAtomicWrite();
		EmitSeparationIfNecessary();
	}

    unsigned Emitter::GetFloatPrecision() const
    {
        return m_pState->GetFloatPrecision();
    }
    
    unsigned Emitter::GetDoublePrecision() const
    {
        return m_pState->GetDoublePrecision();
    }

	void Emitter::PostWriteIntegralType(const std::stringstream& str)
	{
		m_stream << str.str();
		PostAtomicWrite();
	}

	void Emitter::PostWriteStreamable(const std::stringstream& str)
	{
		m_stream << str.str();
		PostAtomicWrite();
	}

	const char *Emitter::ComputeFullBoolName(bool b) const
	{
		const EMITTER_MANIP mainFmt = (m_pState->GetBoolLengthFormat() == ShortBool ? YesNoBool : m_pState->GetBoolFormat());
		const EMITTER_MANIP caseFmt = m_pState->GetBoolCaseFormat();
		switch(mainFmt) {
			case YesNoBool:
				switch(caseFmt) {
					case UpperCase: return b ? "YES" : "NO";
					case CamelCase: return b ? "Yes" : "No";
					case LowerCase: return b ? "yes" : "no";
					default: break;
				}
				break;
			case OnOffBool:
				switch(caseFmt) {
					case UpperCase: return b ? "ON" : "OFF";
					case CamelCase: return b ? "On" : "Off";
					case LowerCase: return b ? "on" : "off";
					default: break;
				}
				break;
			case TrueFalseBool:
				switch(caseFmt) {
					case UpperCase: return b ? "TRUE" : "FALSE";
					case CamelCase: return b ? "True" : "False";
					case LowerCase: return b ? "true" : "false";
					default: break;
				}
				break;
			default:
				break;
		}
		return b ? "y" : "n"; // should never get here, but it can't hurt to give these answers
	}

	Emitter& Emitter::Write(bool b)
	{
		if(!good())
			return *this;
		
		PreAtomicWrite();
		EmitSeparationIfNecessary();
	
		const char *name = ComputeFullBoolName(b);
		if(m_pState->GetBoolLengthFormat() == ShortBool)
			m_stream << name[0];
		else
			m_stream << name;

		PostAtomicWrite();
		return *this;
	}

	Emitter& Emitter::Write(char ch)
	{
		if(!good())
			return *this;
		
		PreAtomicWrite();
		EmitSeparationIfNecessary();
		
		Utils::WriteChar(m_stream, ch);
		
		PostAtomicWrite();
		return *this;
	}

	Emitter& Emitter::Write(const _Alias& alias)
	{
		if(!good())
			return *this;
		
		PreAtomicWrite();
		EmitSeparationIfNecessary();
		if(!Utils::WriteAlias(m_stream, alias.content)) {
			m_pState->SetError(ErrorMsg::INVALID_ALIAS);
			return *this;
		}
		PostAtomicWrite();
		return *this;
	}
	
	Emitter& Emitter::Write(const _Anchor& anchor)
	{
		if(!good())
			return *this;
		
		PreAtomicWrite();
		EmitSeparationIfNecessary();
		if(!Utils::WriteAnchor(m_stream, anchor.content)) {
			m_pState->SetError(ErrorMsg::INVALID_ANCHOR);
			return *this;
		}
		m_pState->RequireHardSeparation();
		// Note: no PostAtomicWrite() because we need another value for this node
		return *this;
	}
	
	Emitter& Emitter::Write(const _Tag& tag)
	{
		if(!good())
			return *this;

		PreAtomicWrite();
		EmitSeparationIfNecessary();
		
		bool success = false;
		if(tag.type == _Tag::Type::Verbatim)
			success = Utils::WriteTag(m_stream, tag.content, true);
		else if(tag.type == _Tag::Type::PrimaryHandle)
			success = Utils::WriteTag(m_stream, tag.content, false);
		else
			success = Utils::WriteTagWithPrefix(m_stream, tag.prefix, tag.content);
		
		if(!success) {
			m_pState->SetError(ErrorMsg::INVALID_TAG);
			return *this;
		}
		
		m_pState->RequireHardSeparation();
		// Note: no PostAtomicWrite() because we need another value for this node
		return *this;
	}

	void Emitter::EmitKindTag()
	{
		Write(LocalTag(""));
	}

	Emitter& Emitter::Write(const _Comment& comment)
	{
		if(!good())
			return *this;
		
		if(m_stream.col() > 0)
			m_stream << Indentation(m_pState->GetPreCommentIndent());
		Utils::WriteComment(m_stream, comment.content, m_pState->GetPostCommentIndent());
		m_pState->RequireHardSeparation();
		m_pState->ForceHardSeparation();
		
		return *this;
	}

	Emitter& Emitter::Write(const _Null& /*null*/)
	{
		if(!good())
			return *this;
		
		PreAtomicWrite();
		EmitSeparationIfNecessary();
		m_stream << "~";
		PostAtomicWrite();
		return *this;
	}

	Emitter& Emitter::Write(const Binary& binary)
	{
		Write(SecondaryTag("binary"));

		if(!good())
			return *this;
		
		PreAtomicWrite();
		EmitSeparationIfNecessary();
		Utils::WriteBinary(m_stream, binary);
		PostAtomicWrite();
		return *this;
	}
}

