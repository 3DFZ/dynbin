# DynBin: Dynamic Binary Library for C
> C언어를 위한 동적 타입 캐스팅 및 동적 함수 호출 라이브러리

## 프로젝트 소개
- DynBin은 순수 C99로 작성된 라이브러리 입니다.
- 단일 소스코드로 손쉽게 include 해서 사용할 수 있습니다.
- 2진 배열 구조체인 `BitArr`를 이용해서 데이터 타입과 관계없이 모든 변수를 `BitArr`에 보관할 수 있습니다.
- `BitArr` 구조체로 변환하는 `PACK`과 원래 데이터 타입으로 캐스팅하는 `UNPACK` 기능을 제공합니다.
- `BitArr` 구조체와 연산 함수들을 사용해서 동적 타입 캐스팅이 가능합니다.
- `PACK`과 `UNPACK` 기능을 사용하면 런타임에 동적으로 함수를 호출할 수 있습니다.
- 동적 함수 호출은 네트워크를 이용한 함수 호출 기술인 RPC(Remote Procedure Call)로써 활용가능 합니다.

## 프로젝트 목표와 해결해야할 과제
- 통신 암호화
- 통신 무결성 검증(현재는 해밍코드 적용)
- 예외 처리, 런타임 에러 처리
- 상태 관리

## 사용법
- 구체적인 사용방법은 Wiki를 참고해 주세요.

## 사용 예제
```C
// example.c

#define DYNBIN_IMPLEMENTATION
#include "dynbin.h"

// 예시1: 덧셈 함수
int add(int a, int b) {
  printf(" call add(%d, %d)\n", a, b);
  return a + b;
} DEF(int, add) ARG(int), ARG(int) DEND

// 예시2: 나누기 함수
double divide(double a, double b) {
  printf(" call divide(%lf, %lf)\n", a, b);
  return a / b;
} DEF(double, divide) ARG(double), ARG(double) DEND

// 예시3: 여러 글자 개수 세기
size_t StringLength(const char* str1, const char* str2) {
  printf(" call StringLength(%s, %s)\n", str1, str2);
  printf(" Pointer StringLength(%llu, %llu)\n", (unsigned long long)str1, (unsigned long long)str2);
  return strlen(str1) + strlen(str2);
} DEF(size_t, StringLength) PTR(const char*), PTR(const char*) DEND

// 보안상 위험하닌깐 금지...
DEF(void*, malloc) ARG(size_t) DPTR  // 리턴값이 포인터인 경우 DPTR을 사용한다.
DEF_VOID(free) PTR(void*) DEND  // 리턴값이 void인 경우 DEF_VOID를 사용한다.

// Main 함수
int main() {
  // 함수 등록
  SET_FN(1, add);
  SET_FN(2, divide);
  SET_FN(3, StringLength);

  // 정적 호출
  printf("static add: %d\n", add(1, 2));
  printf("static divide: %lf\n", divide(1, 2));
  printf("static StringLength: %lu\n", StringLength("hello", "world!"));

  // Arguments
  int add0 = 1, add1 = 2;
  double divide0 = 1, divide1 = 2;
  char str0[] = "hello", str1[] = "world!";

  // 동적 호출
  printf("-------------\n");
  BitArr addArgs[] = { PACK(add0), PACK(add1) };
  BitArr addBitResult = CallFn(NewFnData(1, 2, addArgs));
  printf("dyn add: %d\n", UNPACK(int, addBitResult.bin));

  BitArr divideArgs[] = { PACK(divide0), PACK(divide1) };
  BitArr divideBitResult = CallFn(NewFnData(2, 2, divideArgs));
  printf("dyn divide: %lf\n", UNPACK(double, divideBitResult.bin));

  // 포인터를 인자로 사용할 때는 BitPointerPack을 사용한다.
  BitArr stringLengthArgs[] = { BitPtrPack(str0), BitPtrPack(str1) };
  BitArr stringLengthResult = CallFn(NewFnData(3, 2, stringLengthArgs));
  printf("dyn StringLenght: %lu\n", UNPACK(size_t, stringLengthResult.bin));

  printf("---------------\n");
  // 통신을 통한 동적 호출 - add(1, 2)
  // 1. 함수 호출 요청 생성 (통신 매개를 통해 전송)
  BitArr raw = MakeCallFnBin(NewFnData(1, 2, addArgs));
  // 2. 통신을 통해 받은 요청 처리
  FnData data;
  BitArr response = CallFnBin(raw, &data);

  printf("\nrequest dyn add:\n");
  printf("CallID: %u\n", data.CallID);
  printf("FnID: %u\n", data.FnID);
  printf("Arg Len: %lu\n", data.Args.len);

  // 3. 함수 실행 결과를 다시 요청자에게 전송 (통신 매개를 통해 전송)
  FnData responseData = ResponseFnBin(response);

  printf("\nremote dyn add:\n");
  printf("CallID: %d\n", responseData.CallID);
  printf("FnID: %d\n", responseData.FnID);
  printf("Return: %d\n", UNPACK(int, responseData.Return.bin));

  return 0;
}
```

## "삼디퓨전"팀 소개
> 공방을 담은 스마트 디퓨저, 여러분들의 공간에 소중한 추억을 디자인합니다.
- DynBin 프로젝트는 "삼디퓨전"팀의 프로젝트를 위해 개발되었습니다.

## License
- DynBin은 LGPL 2.1을 적용하고 있습니다.
- 라이선스에 대한 상세한 내용은 [링크](https://olis.or.kr/license/Detailselect.do?lId=1005&mapCode=010005)를 참고해 주세요.
- [라이선스 전문](./LICENSE)
