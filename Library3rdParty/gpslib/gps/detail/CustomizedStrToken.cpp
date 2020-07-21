#include <detail/CustomizedStrToken.hpp>

#include "stdafx.h"

// 在此檔案之外不需要看到此函數
bool _checkCharInDelimiters(char checkedChar, char* delemiters)
{
    int i = 0;
    // 從第一個到最後一個分隔字元逐一檢查
    // 一直到遇到 '\0' 結束符號為止 
    while (delemiters[i] != '\0')
	{
        // 如果待測字元跟目前所在的分隔字元相等的話 ... 
        if (checkedChar == delemiters[i])
		{
            // 已經發現相等 
            return true;
        } 
        // 並不相等
        // 準備測試下一個分隔字元 
        ++i;
    }
    // 所有的分隔字元都不相等 
    return false;
}

void customizedStrtok(char** startParsingPos, char** tokenStartPos, char* delemiters) 
{
    // 如果傳入的待搜尋字串起始位置是空指標 ...
    if (*startParsingPos == NULL) 
	{
        // 把回傳的 token 起始位置也設定成空指標 
        *tokenStartPos = NULL; 
        // 放棄所有搜尋動作而回傳 
        return;
    }  

    // 預設 token 的長度初始值為 0
    // 之後會隨著搜尋結果而有可能增長 
    int tokenLength = 0;
    // 預設 token 的起始位置就是待搜尋字串的起始位置 
    *tokenStartPos = *startParsingPos;
    // 暫存用之字元變數 
    char c;

    // 如果目前正要搜尋的字元不是字串結束符號 ...
    while ((c = (*tokenStartPos)[tokenLength]) != '\0') 
	{ 
        // 檢查是否是跳脫字元 
        if (_checkCharInDelimiters(c, delemiters)) 
		{  
            // 如果是的話就把這個跳脫字元換成字串結束符號
            // 這樣子這個 token 才算是有始有終 
            (*tokenStartPos)[tokenLength] = '\0';
            // 並且把下一個搜尋位置 ( 該位置不會在本函數內被搜尋到 )
            // 指定給 '搜尋字串的起始位置' 的指標變數 
            // 以供下一次呼叫此函數使用
            // ( 否則外部將無法知道下一個 token 要從何找起 ) 
            (*startParsingPos) += (tokenLength +1); 

            // 遇到分隔符號表示 token 已經完整找到
            // 必須跳出函數 
            break; 
        }

        // token 的程度持續增長 
        ++tokenLength;
    }
   
    // 如果上面的搜尋回圈是遇到 '\0' 才停止的 
    // 表示已經搜尋到字串之末了
    // 不需要更進一步的搜尋
    // 所以把 '搜尋字串的起始位置' 的指標變數 
    // 強制設定成 NULL 
    // ( 這樣以後再呼叫這個函數就會被第一個判斷式發現,
    //   而直接跳出 )
    if (c == '\0')
	{
        *startParsingPos = NULL;
    }
  
    // 如果 token 長度為 0 的話
    // 表示 1) 搜尋目標是一個空字串 ( 第一個位置就是 '\0' ) 
    //      2) 本次搜尋的起始位置剛好是跳脫字元 
    // 這兩種情況都沒有 token 可言
    // 所以 token 都設定為 NULL 
    if (tokenLength == 0)
	{
        *tokenStartPos = NULL;
    }
}
