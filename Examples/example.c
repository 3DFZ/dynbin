// example.c

#define DYNBIN_IMPLEMENTATION
#include "../dynbin.h"

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
