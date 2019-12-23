#include <stdio.h>

#define KiloByte 1024

int main(){
    FILE* fp = fopen("testcase.txt","w");
    for(int i=0;i<10*KiloByte;i++){
        char a = '0'+(i%10);
        fprintf(fp,"%c",a);
    }
}