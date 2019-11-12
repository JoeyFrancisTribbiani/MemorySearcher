#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <errno.h>
/* 
    函数：int* MakeSkip(char *, int) 
    目的：根据坏字符规则做预处理，建立一张坏字符表 
    参数： 
        ptrn => 模式串P 
        PLen => 模式串P长度 
    返回： 
        int* - 坏字符表 
*/  
int* MakeSkip(char *ptrn, int pLen)  
{     
    int i;  
    //为建立坏字符表，申请256个int的空间  
    /*PS:之所以要申请256个，是因为一个字符是8位， 
      所以字符可能有2的8次方即256种不同情况*/  
    int *skip = (int*)malloc(256*sizeof(int));  
  
    if(skip == NULL)  
    {  
        fprintf(stderr, "malloc failed!");  
        return 0;  
    }     
  
    //初始化坏字符表，256个单元全部初始化为pLen  
    for(i = 0; i < 256; i++)  
    {  
        *(skip+i) = pLen;  
    }  
  
    //给表中需要赋值的单元赋值，不在模式串中出现的字符就不用再赋值了  
    while(pLen != 0)  
    {  
        *(skip+(unsigned char)*ptrn++) = pLen--;  
    }  
  
    return skip;  
}  
  
  
/* 
    函数：int* MakeShift(char *, int) 
    目的：根据好后缀规则做预处理，建立一张好后缀表 
    参数： 
        ptrn => 模式串P 
        PLen => 模式串P长度 
    返回： 
        int* - 好后缀表 
*/  
int* MakeShift(char* ptrn,int pLen)  
{  
    //为好后缀表申请pLen个int的空间  
    int *shift = (int*)malloc(pLen*sizeof(int));  
    int *sptr = shift + pLen - 1;//方便给好后缀表进行赋值的指标  
    char *pptr = ptrn + pLen - 1;//记录好后缀表边界位置的指标  
    char c;  
  
    if(shift == NULL)  
    {  
        fprintf(stderr,"malloc failed!");  
        return 0;  
    }  
  
    c = *(ptrn + pLen - 1);//保存模式串中最后一个字符，因为要反复用到它  
  
    *sptr = 1;//以最后一个字符为边界时，确定移动1的距离  
  
    pptr--;//边界移动到倒数第二个字符（这句是我自己加上去的，因为我总觉得不加上去会有BUG，大家试试“abcdd”的情况，即末尾两位重复的情况）  
  
    while(sptr-- != shift)//该最外层循环完成给好后缀表中每一个单元进行赋值的工作  
    {  
        char *p1 = ptrn + pLen - 2, *p2,*p3;  
          
        //该do...while循环完成以当前pptr所指的字符为边界时，要移动的距离  
        do{  
            while(p1 >= ptrn && *p1-- != c);//该空循环，寻找与最后一个字符c匹配的字符所指向的位置  
              
            p2 = ptrn + pLen - 2;  
            p3 = p1;  
              
            while(p3 >= ptrn && *p3-- == *p2-- && p2 >= pptr);//该空循环，判断在边界内字符匹配到了什么位置  
  
        }while(p3 >= ptrn && p2 >= pptr);  
  
        *sptr = shift + pLen - sptr + p2 - p3;//保存好后缀表中，以pptr所在字符为边界时，要移动的位置  
        /* 
          PS:在这里我要声明一句，*sptr = （shift + pLen - sptr） + p2 - p3; 
             大家看被我用括号括起来的部分，如果只需要计算字符串移动的距离，那么括号中的那部分是不需要的。 
             因为在字符串自左向右做匹配的时候，指标是一直向左移的，这里*sptr保存的内容，实际是指标要移动 
             距离，而不是字符串移动的距离。我想SNORT是出于性能上的考虑，才这么做的。           
        */  
  
        pptr--;//边界继续向前移动  
    }  
  
    return shift;  
}  
  
  
/* 
    函数：int* BMSearch(char *, int , char *, int, int *, int *) 
    目的：判断文本串T中是否包含模式串P 
    参数： 
        buf => 文本串T 
        blen => 文本串T长度 
        ptrn => 模式串P 
        PLen => 模式串P长度 
        skip => 坏字符表 
        shift => 好后缀表 
    返回： 
        int - 1表示成功（文本串包含模式串），0表示失败（文本串不包含模式串）。 
*/  
int BMSearch(char *buf, int blen, char *ptrn, int plen, int *skip, int *shift)  
{  
    int b_idx = plen;    
    if (plen == 0)  
        return 1;  
    while (b_idx <= blen)//计算字符串是否匹配到了尽头  
    {  
        int p_idx = plen, skip_stride, shift_stride;  
        while (buf[--b_idx] == ptrn[--p_idx])//开始匹配  
        {  
            if (b_idx < 0)  
                return 0;  
            if (p_idx == 0)  
            {       
                return 1;  
            }  
        }  
        skip_stride = skip[(unsigned char)buf[b_idx]];//根据坏字符规则计算跳跃的距离  
        shift_stride = shift[p_idx];//根据好后缀规则计算跳跃的距离  
        b_idx += (skip_stride > shift_stride) ? skip_stride : shift_stride;//取大者  
    }  
    return 0;  
}  
