//
// Created by 진석희 on 2023/06/20.
//
#include <stdio.h>
#include <math.h>

u_int32_t a = 0xf0000001; // 65535
int32_t b = 0xfffffff1; // -1
int32_t c = 0;
u_int32_t d = 0;
// 시나리오 1. 두 언사인드 정수 뺴기와 언사인드 사인드 뺴기. 오버 플로우는 가정해야하나? 오버플로우는 비정상적인 경우라고 생각해야하는거 아닌가?
// imm이 음수값임을 의도했을때, 0xff가 사인익스텐션해서 -1로 읽히는지 확인할것. 저장은 unsigned로 해도 됨. 사인을유지하고 확장되는게 중요.

int main() { // 255이면

    //int16_t c = a;
    printf("unsigned: %lu, signed: %d \n", a, b);
    printf("a-b= %d\n", a-b); // 헐 결과는 그냥 int가 돼버림. 둘다 8비트만 가능한데!
    printf("sign extended %d %d \n", c, d);

    printf("typeof(a): %s \n", );


}