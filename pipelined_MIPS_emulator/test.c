//
// Created by 진석희 on 2023/06/20.
//
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

void test(char *arr);

void test2(u_int32_t *a) {
//    int *newPtr = (u_int32_t *)malloc(sizeof(u_int32_t));
//    *newPtr = 42;  // 임의의 값을 할당
//
//    // 주소를 변경하여 값 전달
    *a = 42;

//    free(newPtr);  // 할당된 메모리 해제
}

int main() { // 255이면
    u_int32_t *alu_control = (u_int32_t *)malloc(sizeof(u_int32_t));
    *alu_control = 0xFFFFFFFF; //
    printf("%d", *alu_control);



}

void test(char *arr){
    strcpy(arr, "asdfef");
}