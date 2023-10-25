#ifndef DYNBIN_H
#define DYNBIN_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>

typedef char BYTE;
#define BOOL BYTE
#define FALSE 0
#define TRUE 1
#define GET(bin, at) (((bin)[(at) / 8] & (BYTE)1 << ((at) % 8)) ? 1 : 0)
#define SET(bin, at) ((bin)[(at) / 8] |= (BYTE)1 << ((at) % 8))
#define UNSET(bin, at) ((bin)[(at) / 8] &= ~((BYTE)1 << ((at) % 8)))
#define TOGGLE(bin, at) ((bin)[(at) / 8] ^= (BYTE)1 << ((at) % 8))
#define PUT(bin, at, val) ((val) ? (SET(bin, at)) : (UNSET(bin, at)))
#define PACK(var) BitPack((void*)(&var), sizeof(var))
#define UNPACK(type, bin) (*(type*)(BitUnpack(bin, sizeof(type))))

#define FN_INDEX unsigned int
#define FN_MAX_COUNT sizeof(FN_INDEX)
#define FN_ARG_INDEX short
#define FN_MAX_ARG_COUNT sizeof(FN_ARG_INDEX)
#define CHAR sizeof(char)
#define INT sizeof(int)
#define SHORT sizeof(short)
#define SET_FN(fid, name) SetFn(fid, WRAP_##name)
#define CALL_FN(fn, ...) fn(__VA_ARGS__)
#define ARG(type) UNPACK(type, BitArg(&args, sizeof(type)).bin)
#define PTR(type) (type)ARG(unsigned long long)
#define BIT(size) BitArg(&arg, size)
#define DEF(ret, name) \
BitArr WRAP_##name(BitArr args) { \
  ret RET = name(
#define DEF_VOID(name) \
BitArr WRAP_##name(BitArr args) { \
  int RET = 0;\
  name(
#define DEND ); return PACK(RET); }
#define DPTR ); return BitPtrPack(RET); }
#define DBIT ); return RET; }

#define MESSAGE_TYPE_REQUEST_FN_CALL ((unsigned short)(1))
#define MESSAGE_TYPE_RESPONSE_FN_CALL ((unsigned short)(2))

typedef struct {
  BYTE* bin;
  size_t len;
} BitArr;

typedef BitArr (*DynFn)(BitArr);
typedef struct {
  FN_INDEX CallID;
  FN_INDEX FnID;
  unsigned short MessageType;
  BitArr Args;
  BitArr Err;
  BitArr Return;
} FnData;

void SetFn(FN_INDEX fid, DynFn callback);
BitArr CallFn(FnData data);
FnData NewFnData(FN_INDEX fnID, size_t argLen, BitArr args[]);
BitArr MakeCallFnBin(FnData data);
BitArr CallFnBin(BitArr bit, FnData *data);
FnData ResponseFnBin(BitArr bit);

BitArr NewBit(size_t len);
BitArr Str2Bit(const char str[]);
BitArr BitPack(void* data, size_t bytes);
BitArr BitPtrPack(void* ptr);
void* BitUnpack(BYTE* bit, size_t bytes);
void BitReset(BitArr bit);
BitArr BitSlice(BitArr bit, size_t start, size_t end);
BitArr BitAppend(BitArr bitA, BitArr bitB);
BitArr BitJoin(size_t arrLen, BitArr bits[], BitArr sep);
BitArr BitArg(BitArr *bit, size_t size);

BitArr HammingCode(BitArr bits, BYTE odd);
BOOL HammingCodeCheck(BitArr bits, BYTE odd);
BitArr HammingCodeData(BitArr bits);

#endif





#ifdef DYNBIN_IMPLEMENTATION
#define DYNBIN_IMPLEMENTATION

////////////////////////////////////////////////////////////////////////////
// HammingCode
////////////////////////////////////////////////////////////////////////////
BitArr HammingCode(BitArr bits, BYTE odd) {
  // Minimum Parity Bit Count
  int parityCount = 0;
  while (pow(2, parityCount) < bits.len + parityCount + 1) ++parityCount;

  // Create New Bit Array
  BitArr code = NewBit(bits.len + parityCount);

  int parityIndex = 0;
  for (size_t bitIndex = 0, codeIndex = 0; bitIndex < bits.len; ++bitIndex, ++codeIndex) {
    if (codeIndex == pow(2, parityIndex) - 1) {
      ++parityIndex;
      --bitIndex;
      UNSET(code.bin, codeIndex);
    } else {
      PUT(code.bin, codeIndex, GET(bits.bin, bitIndex));
    }
  }

  // Hamming Code
  for (int currentParity = 0; currentParity < parityCount; ++currentParity) {
    unsigned int paritySize = pow(2, currentParity);
    BYTE toggle = 0;

    for (
      unsigned int index = paritySize - 1, subIndex = 0;
      index < code.len;
      ++index, ++subIndex
    ) {
      if (subIndex >= paritySize) {
        index += paritySize;
        subIndex = 0;
      }
      if (GET(code.bin, index)) {
        toggle = !toggle;
      }
    }

    PUT(code.bin, paritySize - 1, odd ? !toggle : toggle);
  }

  return code;
}

BOOL HammingCodeCheck(BitArr bits, BYTE odd) {
  // Hamming Code
  BitArr check = NewBit(0);
  int currentParity = 0;
  unsigned int paritySize = pow(2, currentParity++);

  do {
    check = BitAppend(check, Str2Bit("0"));
    BYTE toggle = 0;

    for (
      unsigned int index = paritySize - 1, subIndex = 0;
      index < bits.len;
      ++index, ++subIndex
    ) {
      if (subIndex >= paritySize) {
        index += paritySize;
        subIndex = 0;
      }
      if (GET(bits.bin, index)) {
        toggle = !toggle;
      }
    }

    PUT(check.bin, currentParity, odd ? !toggle : toggle);
    paritySize = pow(2, currentParity++);
  } while (paritySize <= bits.len);
  
  BYTE error = check.bin[0];
  if (error) {
    TOGGLE(bits.bin, error - 1);
  }

  return error;
}

BitArr HammingCodeData(BitArr bits) {
  // Create New Bit Array
  BitArr decode = NewBit(bits.len);

  int currentParity = 0;
  unsigned int paritySize = 1;

  for (
    size_t bitIndex = 0, decodeIndex = 0;
    bitIndex < bits.len;
    ++bitIndex, ++decodeIndex
  ) {
    if (bitIndex == paritySize - 1) {
      paritySize = pow(2, ++currentParity);
      --decodeIndex;
    } else {
      PUT(decode.bin, decodeIndex, GET(bits.bin, bitIndex));
    }
  }

  decode.len -= currentParity;
  return decode;
}

////////////////////////////////////////////////////////////////////////////
// BitArr Operators
////////////////////////////////////////////////////////////////////////////
BitArr NewBit(size_t len) {
  BitArr bits = {};
  bits.bin = (BYTE*)malloc(sizeof(BYTE) * ceil(len / 8));
  bits.len = len;
  return bits;
}

BitArr Str2Bit(const char str[]) {
  BitArr bits = NewBit(strlen(str));
  for (size_t i = 0; i < bits.len; ++i) {
    PUT(bits.bin, i, str[i] == '1');
  }
  return bits;
}

BitArr BitPack(void* data, size_t bytes) {
  BitArr pack = NewBit(bytes * 8);
  for (size_t i = 0; i < bytes * 8; ++i) {
    PUT(pack.bin, i, GET((BYTE*)data, i));
  }
  return pack;
}

BitArr BitPtrPack(void* ptr) {
  unsigned long long mem = (unsigned long long)ptr;
  return PACK(mem);
}

void* BitUnpack(BYTE* bit, size_t bytes) {
  BitArr unpack = NewBit(bytes * 8);
  for (size_t i = 0; i < bytes * 8; ++i) {
    PUT(unpack.bin, i, GET(bit, i));
  }
  return unpack.bin;
}

void BitReset(BitArr bit) {
  size_t i = 0;
  for (i = 0; i * 8 < bit.len; ++i) {
    bit.bin[i] = 0;
  }
  bit.bin[i] = 0;
}

BitArr BitSlice(BitArr bit, size_t start, size_t end) {
  const size_t size = end - start;
  BitArr slice = NewBit(size);
  for (size_t i = 0, j = start; j < end; ++i, ++j) {
    PUT(slice.bin, i, GET(bit.bin, j));
  }
  return slice;
}

size_t BitFind(BitArr bit, BitArr find) {
  size_t i = 0, j = 0;
  for (; i < bit.len; ++i) {
    if (j == find.len) {
      break;
    } else if (GET(bit.bin, i) == GET(find.bin, j)) {
      ++j;
    } else {
      j = 0;
    }
  }
  return i - find.len;
}

BitArr BitAppend(BitArr bitA, BitArr bitB) {
    BitArr result = NewBit(bitA.len + bitB.len);
    for (size_t i = 0; i < bitA.len; ++i) {
        PUT(result.bin, i, GET(bitA.bin, i));
    }
    for (size_t i = 0; i < bitB.len; ++i) {
        PUT(result.bin, bitA.len + i, GET(bitB.bin, i));
    }
    return result;
}

BitArr BitJoin(size_t arrLen, BitArr bits[], BitArr sep) {
    BitArr result = BitSlice(bits[0], 0, bits[0].len);
    for (size_t i = 1; i < arrLen; ++i) {
        result = BitAppend(result, sep);
        result = BitAppend(result, bits[i]);
    }
    return result;
}

BitArr BitArg(BitArr *bit, size_t bytes) {
  size_t size = bytes * 8;
  BitArr arg = BitSlice(*bit, 0, size);
  BitArr result = BitSlice(*bit, size, bit->len);
  *bit = result;
  return arg;
}

////////////////////////////////////////////////////////////////////////////
// Dynamic Function Call
////////////////////////////////////////////////////////////////////////////
DynFn FN[FN_MAX_COUNT];
FN_INDEX FN_CALL_COUNT = 0;
FN_INDEX FN_CALLING[FN_MAX_COUNT];

void SetFn(FN_INDEX fid, DynFn callback) {
  FN[fid] = callback;
}

BitArr CallFn(FnData data) {
  return FN[data.FnID](data.Args);
}

FnData NewFnData(FN_INDEX fnID, size_t argLen, BitArr args[]) {
  FnData data = {};
  data.FnID = fnID;
  data.Args = BitJoin(argLen, args, NewBit(0));
  data.MessageType = MESSAGE_TYPE_REQUEST_FN_CALL;
  return data;
}

/*
  Request Call Function
  +--------+--------+----------+---------+---------+------+--------+
  | STX(8) | SOH(8) | TYPE(16) | CID(32) | FID(32) | Args | ETX(8) |
  +--------+--------+----------+---------+---------+------+--------+

  Response Function Return
  +--------+--------+----------+---------+--------+--------+--------+
  | STX(8) | SOH(8) | TYPE(16) | CID(32) | ERR(8) | Return | ETX(8) |
  +--------+--------+----------+---------+--------+--------+--------+

    Hamming Code(Even)

    STX: Start of Text (ASCII CODE)
    SOH: Start of Header (ASCII CODE)
    ETX: End of Text (ASCII CODE)

    TYPE: Message Type (0:Reserved, 1:Request Call Function, 2:Response Function Return, 3-end:Reserved)
    CID: Call ID (unsigend int - 4 bytes)
    FID: Function ID (unsigend int - 4 bytes)
    ERR: Call Function Error (0:OK, 1:BadRequest, 2:RuntimeError, 3:UnkownError, 4-7:Reserved)
*/
BitArr MakeCallFnBin(FnData data) {
  data.CallID = ++FN_CALL_COUNT;
  FN_CALLING[data.CallID] = data.FnID;

  BitArr bits[] = {
    Str2Bit("00000010"),  // STX
    Str2Bit("00000001"),  // SOH
    PACK(data.MessageType),
    PACK(data.CallID),
    PACK(data.FnID),
    data.Args,
    Str2Bit("00000011"),  // ETX
  };

  BitArr code = BitJoin(6, bits, NewBit(0));
  return HammingCode(code, FALSE);
}

BitArr CallFnBin(BitArr raw, FnData *data) {
  // Hamming Code (Even)
  HammingCodeCheck(raw, FALSE);
  BitArr bit = HammingCodeData(raw);

  // Parsing
  data->CallID = UNPACK(FN_INDEX, BitSlice(bit, 32, 64).bin);
  data->FnID = UNPACK(FN_INDEX, BitSlice(bit, 64, 96).bin);

  size_t etxIndex = BitFind(bit, Str2Bit("00000011"));
  data->Args = BitSlice(bit, 96, etxIndex);

  // CallFn
  data->Return = CallFn(*data);
  data->Err = Str2Bit("00000000");

  // Response
  unsigned short responseMessageType = MESSAGE_TYPE_RESPONSE_FN_CALL;
  BitArr response[] = {
    Str2Bit("00000010"),  // STX
    Str2Bit("00000001"),  // SOH
    PACK(responseMessageType),
    PACK(data->CallID),
    data->Err,
    data->Return,
    Str2Bit("00000011"),  // ETX
  };

  BitArr code = BitJoin(6, response, NewBit(0));
  return HammingCode(code, FALSE);
}

FnData ResponseFnBin(BitArr raw) {
  // Hamming Code (Even)
  HammingCodeCheck(raw, FALSE);
  BitArr bit = HammingCodeData(raw);

  // Parsing
  FnData data;
  data.CallID = UNPACK(FN_INDEX, BitSlice(bit, 32, 64).bin);
  data.Err = BitSlice(bit, 64, 72);
  size_t etxIndex = BitFind(bit, Str2Bit("00000011"));
  data.Return = BitSlice(bit, 72, etxIndex);

  data.FnID = FN_CALLING[data.CallID];
  FN_CALLING[data.CallID] = 0;

  return data;
}

////////////////////////////////////////////////////////////////////////////
// For Non Arduino C
////////////////////////////////////////////////////////////////////////////
#ifndef ARDUINO_C
void BitPrint(BitArr bits);
void BitPrintln(BitArr bits);

void BitPrint(BitArr bits) {
  for (size_t i = 0; i < bits.len; ++i) {
    printf("%c", GET(bits.bin, i) ? '1' : '0');
  }
}

void BitPrintln(BitArr bits) {
  BitPrint(bits);
  printf("\n");
}
#endif

////////////////////////////////////////////////////////////////////////////
// For Arduino C
////////////////////////////////////////////////////////////////////////////
#ifdef ARDUINO_C
void ReadFromSerial() {}
#endif

#endif
