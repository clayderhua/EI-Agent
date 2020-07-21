#include <detail/CustomizedStrToken.hpp>

#include "stdafx.h"

// �b���ɮפ��~���ݭn�ݨ즹���
bool _checkCharInDelimiters(char checkedChar, char* delemiters)
{
    int i = 0;
    // �q�Ĥ@�Ө�̫�@�Ӥ��j�r���v�@�ˬd
    // �@����J�� '\0' �����Ÿ����� 
    while (delemiters[i] != '\0')
	{
        // �p�G�ݴ��r����ثe�Ҧb�����j�r���۵����� ... 
        if (checkedChar == delemiters[i])
		{
            // �w�g�o�{�۵� 
            return true;
        } 
        // �ä��۵�
        // �ǳƴ��դU�@�Ӥ��j�r�� 
        ++i;
    }
    // �Ҧ������j�r�������۵� 
    return false;
}

void customizedStrtok(char** startParsingPos, char** tokenStartPos, char* delemiters) 
{
    // �p�G�ǤJ���ݷj�M�r��_�l��m�O�ū��� ...
    if (*startParsingPos == NULL) 
	{
        // ��^�Ǫ� token �_�l��m�]�]�w���ū��� 
        *tokenStartPos = NULL; 
        // ���Ҧ��j�M�ʧ@�Ӧ^�� 
        return;
    }  

    // �w�] token �����ת�l�Ȭ� 0
    // ����|�H�۷j�M���G�Ӧ��i��W�� 
    int tokenLength = 0;
    // �w�] token ���_�l��m�N�O�ݷj�M�r�ꪺ�_�l��m 
    *tokenStartPos = *startParsingPos;
    // �Ȧs�Τ��r���ܼ� 
    char c;

    // �p�G�ثe���n�j�M���r�����O�r�굲���Ÿ� ...
    while ((c = (*tokenStartPos)[tokenLength]) != '\0') 
	{ 
        // �ˬd�O�_�O����r�� 
        if (_checkCharInDelimiters(c, delemiters)) 
		{  
            // �p�G�O���ܴN��o�Ӹ���r�������r�굲���Ÿ�
            // �o�ˤl�o�� token �~��O���l���� 
            (*tokenStartPos)[tokenLength] = '\0';
            // �åB��U�@�ӷj�M��m ( �Ӧ�m���|�b����Ƥ��Q�j�M�� )
            // ���w�� '�j�M�r�ꪺ�_�l��m' �������ܼ� 
            // �H�ѤU�@���I�s����ƨϥ�
            // ( �_�h�~���N�L�k���D�U�@�� token �n�q���_ ) 
            (*startParsingPos) += (tokenLength +1); 

            // �J����j�Ÿ���� token �w�g������
            // �������X��� 
            break; 
        }

        // token ���{�׫���W�� 
        ++tokenLength;
    }
   
    // �p�G�W�����j�M�^��O�J�� '\0' �~��� 
    // ��ܤw�g�j�M��r�ꤧ���F
    // ���ݭn��i�@�B���j�M
    // �ҥH�� '�j�M�r�ꪺ�_�l��m' �������ܼ� 
    // �j��]�w�� NULL 
    // ( �o�˥H��A�I�s�o�Ө�ƴN�|�Q�Ĥ@�ӧP�_���o�{,
    //   �Ӫ������X )
    if (c == '\0')
	{
        *startParsingPos = NULL;
    }
  
    // �p�G token ���׬� 0 ����
    // ��� 1) �j�M�ؼЬO�@�ӪŦr�� ( �Ĥ@�Ӧ�m�N�O '\0' ) 
    //      2) �����j�M���_�l��m��n�O����r�� 
    // �o��ر��p���S�� token �i��
    // �ҥH token ���]�w�� NULL 
    if (tokenLength == 0)
	{
        *tokenStartPos = NULL;
    }
}
