#ifndef _CUSTOMIZED_STR_TOKEN_H_
#define _CUSTOMIZED_STR_TOKEN_H_

// 根據 '搜尋字串的起始位置' startParsingPos
// 以 '分隔字元字串' delemiters 逐一進行判斷 
// 如果遇到 '分隔字串' 
// 表示之前的所有已搜尋字元可成為一個單獨的 token
// 此 token 的起始位置紀錄在 tokenStartPos 當中 
// 如果 tokenStartPos == NULL 表示找不到 token
// 如果 startParsingPos == NULL 表示此字串已經徹底被搜尋完畢 
// ( 繼續再搜尋也不會找到任何東西, 可參考程式碼 )
//
// 大抵上此函數類似標準函式庫之 strtok
// 但是實作上與功能上略有不同 
void customizedStrtok(char** startParsingPos, char** tokenStartPos, 
    char* delemiters);

#endif
