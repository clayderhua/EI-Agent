#ifndef _XML_HELPER_H_
#define _XML_HELPER_H_

typedef void * PXmlDoc;

#ifdef __cplusplus
extern "C" {
#endif

PXmlDoc CreateXmlDoc();

PXmlDoc CreateXmlDocFromComment(const char * comment, int size);

PXmlDoc CreateXmlDocFromFile(char * cfgPath);

int GetItemValueFormDoc(PXmlDoc doc, char * itemPath, char * itemValue, unsigned int valueLen);

int GetItemValueFormDocEx(PXmlDoc doc, char * itemPath, char ** itemValue);

int SetItemValueToDoc(PXmlDoc doc, const char * itemPath, const char * itemValue);

int SaveXmlDocToFile(PXmlDoc doc, char * cfgPath);

void DestoryXmlDoc(PXmlDoc doc);

#ifdef __cplusplus
};
#endif

#endif

