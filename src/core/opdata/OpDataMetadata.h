/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef INCLUDED_OCIO_OPDATAMETADATA_H
#define INCLUDED_OCIO_OPDATAMETADATA_H

#include <OpenColorIO/OpenColorIO.h>

#include <list>
#include <string>
#include <vector>

OCIO_NAMESPACE_ENTER
{
    // Private namespace to the OpData sub-directory
    namespace OpData
    {
        //This class provides an hierarchical, name-associative metadata container.
        class Metadata
        {
        public:
            typedef std::list<Metadata> MetadataList;
            typedef std::list<std::string> NameList;
            typedef std::pair<std::string,std::string> Attribute;
            typedef std::vector<Attribute> Attributes;

            //Contructor
            // - name the name of the metadata item
            Metadata(const std::string & name);

            //Copy Contructor
            // - other the metadata item to copy
            Metadata(const Metadata & other);

            // Retrieve the name of the metadata item
            // Return the name of the metadata item
            const std::string& getName() const;

            // Retrieve the value of a leaf metadata item
            // An exception is thrown if the metadata item is not a leaf element
            // Return the value of the metadata item
            const std::string& getValue() const;

            // Retrieve the list of attributes
            // Return the attributes of the metadata item
            const Attributes& getAttributes() const;

            // Assign the given attribute to a metadata element. 
            // - attribute the attribute to be assigned to the metadata element.
            // Note: If the attribute already exists, the existing attribute's
            //       value will be overwritten.
            void addAttribute(const Attribute & attribute);

            //Retrieve the list of items under the metadata
            // An exception is thrown if the metadata item is not a container element
            // Return The list of items under the metadata
            MetadataList getItems() const;

            // Retrieve the name of the metadata items under the metadata
            // An exception is thrown if the metadata item is not a container element
            // Return the names of items under the metadata
            NameList getItemsNames() const;

            // Verifiy if the metadata item is a left element
            // Return true if the metadata is a leaf element, false otherwise
            bool isLeaf() const;

            // Verifiy if the metadata item is an empty element, that is,
            // it is an empty string and has no children metadata.
            // Return true if the metadata is empty, false otherwise
            bool isEmpty() const;

            // Reset the contents of a metadata item. Both value and list of items of
            // the metadata are cleared. This automatically makes the metadata item an
            // empty leaf element.
            void clear();

            // Remove the metadata with a given name from the list of items.
            // An exception is thrown if no metadata item with the given name
            // is in the list of items.
            // - name the name of the metadata item to be removed
            void remove(const std::string & name);

            // Access a metadata element in the list of items. If a metadata item 
            // with the given exists, a reference to it is returned. If the given name 
            // does not match the name of any metadata item, a new element is inserted 
            // with that name and a reference to the name item is returned.
            // - name the name of the metadata element to be retrieved
            // Return a reference to the metadata item with the given name
            Metadata& operator[](const std::string & name);

            // Access a metadata element in the list of items. If a metadata item with 
            // the given exists, a reference to it is returned. If the given name 
            // does not match the name of any metadata item, an exception is thrown.
            // - name the name of the metadata element to be retrieved
            // Return a reference to the metadata item with the given name
            const Metadata& operator[](const std::string & name) const;

            // Assign the given value to a metadata element. If the metadata element 
            // is a container, the children items are cleared and the element becomes
            // automatically a leaf metadata element.
            // - value the value to be assigned to the metadata element.
            // Return the metadata element
            Metadata& operator=(const std::string & value);

            // Copy assignment operator
            // - rhs the metadata element to copy.
            // Return the metadata element
            Metadata& operator=(const Metadata & rhs);

        private:
            // Prevent the creation of a metadata element without a name
            Metadata();

            std::string  m_name;       // The element name
            std::string  m_value;      // The element value
            Attributes   m_attributes; // The element's list of attributes
            MetadataList m_items;      // The list of subelements
        };
    }
}
OCIO_NAMESPACE_EXIT

#endif
