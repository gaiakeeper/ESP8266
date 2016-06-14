#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"

#include "yxml2.h"

XML_Element* xmlNewElement(void)
{
	XML_Element* element = (XML_Element *)os_malloc(sizeof(XML_Element));
	os_memset((void*)element, 0, sizeof(XML_Element));

	return element;
}

void xmlDeleteElement(XML_Element* element)
{
	if(element){
		xmlDeleteElement(element->child);
		xmlDeleteElement(element->sibling);

		os_free(element->name);
		os_free(element->value);

		os_free(element);
	}
}

XML_Document* xmlNewDocument(void)
{
	return (XML_Document*)xmlNewElement();
}

void xmlDeleteDocument(XML_Document* doc)
{
	xmlDeleteElement((XML_Element*)doc);
}

XML_Element* xmlFindNextSibling(XML_Element* element, const char* name)
{
	XML_Element* sibling = xmlSibling(element);
	if(name==NULL || sibling==NULL) return sibling;

	for(;sibling!=NULL;sibling=sibling->sibling)
		if(os_strcmp(sibling->name, name)==0) break;
	return sibling;
}

XML_Element* xmlFindFirstChild(XML_Element* element, const char* name)
{
	XML_Element* child = xmlChild(element);
	if(name==NULL || child==NULL) return child;
	if(os_strcmp(child->name, name)==0) return child;

	return xmlFindNextSibling(child, name);
}

XML_Element* current = NULL;
char _buf[MAX_VALUE_LENGTH+1];
int _length=0;
void xmlConstructTree(yxml_t *x, yxml_ret_t r)
{
	switch(r) {
		case YXML_OK:
			break;
		case YXML_ELEMSTART:
			{
				if(current->child){
					XML_Element* sibling = xmlNewElement();
					sibling->parent = current;
					sibling->sibling = current->child;
					current->child = sibling;
					current = sibling;
				}
				else {
					XML_Element* child = xmlNewElement();
					child->parent = current;
					current->child = child;
					current = child;
				}
				current->name = (char*)os_malloc(os_strlen(x->elem)+1);
				os_strcpy(current->name, x->elem);
				_length = 0;
			}
			break;
		case YXML_ELEMEND:
			if(_length>0){
				_buf[_length] = 0;
				current->value = (char*)os_malloc(_length+1);
				os_strcpy(current->value, _buf);
				_length = 0;
			}
			current = current->parent;
			break;
		case YXML_CONTENT:
			if(_length < MAX_VALUE_LENGTH) _buf[_length++] = x->data[0];
			break;
		case YXML_ATTRSTART:		// ignore attribute
		case YXML_ATTREND:			// ignore attribute
		case YXML_ATTRVAL:
		case YXML_PISTART:			// ignore pi
		case YXML_PIEND:			// ignore pi
		case YXML_PICONTENT:
		default:
			break;
	}
}

static char stack[1024];
BOOL xmlParse(XML_Document* doc, const char* data, unsigned int length)
{
	yxml_t x;
	yxml_ret_t r;
	int i=0;

	yxml_init(&x, stack, sizeof(stack));

	current = (XML_Element*)doc;

	for(i=0; i<length; i++){
		r = yxml_parse(&x, data[i]);
		xmlConstructTree(&x, r);
	}

	return (current==doc);
}
