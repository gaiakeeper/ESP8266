#ifndef ___YXML2_H___
#define ___YXML2_H___

#include "yxml.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _XML_Element {
	struct _XML_Element *parent, *child, *sibling;
	char* name;
	char* value;
} XML_Element;
typedef XML_Element XML_Document;
#define xmlParent(element)	((element)->parent)
#define xmlChild(element)	((element)->child)
#define xmlSibling(element)	((element)->sibling)
#define xmlName(element)	((element)->name)
#define xmlValue(element)	((element)->value)

XML_Document* xmlNewDocument(void);
void xmlDeleteDocument(XML_Document* doc);
#define xmlRootElement(doc)	(XML_Element*)((doc)->child)

XML_Element* xmlFindFirstChild(XML_Element* element, const char* name);
XML_Element* xmlFindNextSibling(XML_Element* element, const char* name);

BOOL xmlParse(XML_Document* doc, const char* data, unsigned int length);

#define MAX_VALUE_LENGTH 128

#ifdef __cplusplus
}
#endif

#endif // ___YXML2_H___
