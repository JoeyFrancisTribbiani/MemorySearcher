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
    ������int* MakeSkip(char *, int) 
    Ŀ�ģ����ݻ��ַ�������Ԥ��������һ�Ż��ַ��� 
    ������ 
        ptrn => ģʽ��P 
        PLen => ģʽ��P���� 
    ���أ� 
        int* - ���ַ��� 
*/  
int* MakeSkip(char *ptrn, int pLen)  
{     
    int i;  
    //Ϊ�������ַ�������256��int�Ŀռ�  
    /*PS:֮����Ҫ����256��������Ϊһ���ַ���8λ�� 
      �����ַ�������2��8�η���256�ֲ�ͬ���*/  
    int *skip = (int*)malloc(256*sizeof(int));  
  
    if(skip == NULL)  
    {  
        fprintf(stderr, "malloc failed!");  
        return 0;  
    }     
  
    //��ʼ�����ַ���256����Ԫȫ����ʼ��ΪpLen  
    for(i = 0; i < 256; i++)  
    {  
        *(skip+i) = pLen;  
    }  
  
    //��������Ҫ��ֵ�ĵ�Ԫ��ֵ������ģʽ���г��ֵ��ַ��Ͳ����ٸ�ֵ��  
    while(pLen != 0)  
    {  
        *(skip+(unsigned char)*ptrn++) = pLen--;  
    }  
  
    return skip;  
}  
  
  
/* 
    ������int* MakeShift(char *, int) 
    Ŀ�ģ����ݺú�׺������Ԥ��������һ�źú�׺�� 
    ������ 
        ptrn => ģʽ��P 
        PLen => ģʽ��P���� 
    ���أ� 
        int* - �ú�׺�� 
*/  
int* MakeShift(char* ptrn,int pLen)  
{  
    //Ϊ�ú�׺������pLen��int�Ŀռ�  
    int *shift = (int*)malloc(pLen*sizeof(int));  
    int *sptr = shift + pLen - 1;//������ú�׺����и�ֵ��ָ��  
    char *pptr = ptrn + pLen - 1;//��¼�ú�׺��߽�λ�õ�ָ��  
    char c;  
  
    if(shift == NULL)  
    {  
        fprintf(stderr,"malloc failed!");  
        return 0;  
    }  
  
    c = *(ptrn + pLen - 1);//����ģʽ�������һ���ַ�����ΪҪ�����õ���  
  
    *sptr = 1;//�����һ���ַ�Ϊ�߽�ʱ��ȷ���ƶ�1�ľ���  
  
    pptr--;//�߽��ƶ��������ڶ����ַ�����������Լ�����ȥ�ģ���Ϊ���ܾ��ò�����ȥ����BUG��������ԡ�abcdd�����������ĩβ��λ�ظ��������  
  
    while(sptr-- != shift)//�������ѭ����ɸ��ú�׺����ÿһ����Ԫ���и�ֵ�Ĺ���  
    {  
        char *p1 = ptrn + pLen - 2, *p2,*p3;  
          
        //��do...whileѭ������Ե�ǰpptr��ָ���ַ�Ϊ�߽�ʱ��Ҫ�ƶ��ľ���  
        do{  
            while(p1 >= ptrn && *p1-- != c);//�ÿ�ѭ����Ѱ�������һ���ַ�cƥ����ַ���ָ���λ��  
              
            p2 = ptrn + pLen - 2;  
            p3 = p1;  
              
            while(p3 >= ptrn && *p3-- == *p2-- && p2 >= pptr);//�ÿ�ѭ�����ж��ڱ߽����ַ�ƥ�䵽��ʲôλ��  
  
        }while(p3 >= ptrn && p2 >= pptr);  
  
        *sptr = shift + pLen - sptr + p2 - p3;//����ú�׺���У���pptr�����ַ�Ϊ�߽�ʱ��Ҫ�ƶ���λ��  
        /* 
          PS:��������Ҫ����һ�䣬*sptr = ��shift + pLen - sptr�� + p2 - p3; 
             ��ҿ������������������Ĳ��֣����ֻ��Ҫ�����ַ����ƶ��ľ��룬��ô�����е��ǲ����ǲ���Ҫ�ġ� 
             ��Ϊ���ַ�������������ƥ���ʱ��ָ����һֱ�����Ƶģ�����*sptr��������ݣ�ʵ����ָ��Ҫ�ƶ� 
             ���룬�������ַ����ƶ��ľ��롣����SNORT�ǳ��������ϵĿ��ǣ�����ô���ġ�           
        */  
  
        pptr--;//�߽������ǰ�ƶ�  
    }  
  
    return shift;  
}  
  
  
/* 
    ������int* BMSearch(char *, int , char *, int, int *, int *) 
    Ŀ�ģ��ж��ı���T���Ƿ����ģʽ��P 
    ������ 
        buf => �ı���T 
        blen => �ı���T���� 
        ptrn => ģʽ��P 
        PLen => ģʽ��P���� 
        skip => ���ַ��� 
        shift => �ú�׺�� 
    ���أ� 
        int - 1��ʾ�ɹ����ı�������ģʽ������0��ʾʧ�ܣ��ı���������ģʽ������ 
*/  
int BMSearch(char *buf, int blen, char *ptrn, int plen, int *skip, int *shift)  
{  
    int b_idx = plen;    
    if (plen == 0)  
        return 1;  
    while (b_idx <= blen)//�����ַ����Ƿ�ƥ�䵽�˾�ͷ  
    {  
        int p_idx = plen, skip_stride, shift_stride;  
        while (buf[--b_idx] == ptrn[--p_idx])//��ʼƥ��  
        {  
            if (b_idx < 0)  
                return 0;  
            if (p_idx == 0)  
            {       
                return 1;  
            }  
        }  
        skip_stride = skip[(unsigned char)buf[b_idx]];//���ݻ��ַ����������Ծ�ľ���  
        shift_stride = shift[p_idx];//���ݺú�׺���������Ծ�ľ���  
        b_idx += (skip_stride > shift_stride) ? skip_stride : shift_stride;//ȡ����  
    }  
    return 0;  
}  
