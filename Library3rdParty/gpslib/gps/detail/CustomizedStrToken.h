#ifndef _CUSTOMIZED_STR_TOKEN_H_
#define _CUSTOMIZED_STR_TOKEN_H_

// �ھ� '�j�M�r�ꪺ�_�l��m' startParsingPos
// �H '���j�r���r��' delemiters �v�@�i��P�_ 
// �p�G�J�� '���j�r��' 
// ��ܤ��e���Ҧ��w�j�M�r���i�����@�ӳ�W�� token
// �� token ���_�l��m�����b tokenStartPos �� 
// �p�G tokenStartPos == NULL ��ܧ䤣�� token
// �p�G startParsingPos == NULL ��ܦ��r��w�g�����Q�j�M���� 
// ( �~��A�j�M�]���|������F��, �i�Ѧҵ{���X )
//
// �j��W����������зǨ禡�w�� strtok
// ���O��@�W�P�\��W�������P 
void customizedStrtok(char** startParsingPos, char** tokenStartPos, 
    char* delemiters);

#endif
