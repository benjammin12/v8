// Copyright 2014 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function(global, exports) {

"use strict";

%CheckIsBootstrapping();

macro TYPED_ARRAYS(FUNCTION)
// arrayIds below should be synchronized with Runtime_TypedArrayInitialize.
FUNCTION(Uint8Array)
FUNCTION(Int8Array)
FUNCTION(Uint16Array)
FUNCTION(Int16Array)
FUNCTION(Uint32Array)
FUNCTION(Int32Array)
FUNCTION(Float32Array)
FUNCTION(Float64Array)
FUNCTION(Uint8ClampedArray)
endmacro

macro DECLARE_GLOBALS(NAME)
var GlobalNAME = global.NAME;
endmacro

TYPED_ARRAYS(DECLARE_GLOBALS)
DECLARE_GLOBALS(Array)

// -------------------------------------------------------------------

function ConstructTypedArray(constructor, array) {
  // TODO(littledan): This is an approximation of the spec, which requires
  // that only real TypedArray classes should be accepted (22.2.2.1.1)
  if (!IS_SPEC_OBJECT(constructor) || IS_UNDEFINED(constructor.prototype) ||
      !%HasOwnProperty(constructor.prototype, "BYTES_PER_ELEMENT")) {
    throw MakeTypeError(kNotTypedArray);
  }

  // TODO(littledan): The spec requires that, rather than directly calling
  // the constructor, a TypedArray is created with the proper proto and
  // underlying size and element size, and elements are put in one by one.
  // By contrast, this would allow subclasses to make a radically different
  // constructor with different semantics.
  return new constructor(array);
}

function ConstructTypedArrayLike(typedArray, arrayContents) {
  // TODO(littledan): The spec requires that we actuallly use
  // typedArray.constructor[Symbol.species] (bug v8:4093)
  return new typedArray.constructor(arrayContents);
}

function TypedArrayCopyWithin(target, start, end) {
  if (!%IsTypedArray(this)) throw MakeTypeError(kNotTypedArray);

  var length = %_TypedArrayGetLength(this);

  // TODO(littledan): Replace with a memcpy for better performance
  return $innerArrayCopyWithin(target, start, end, this, length);
}
%FunctionSetLength(TypedArrayCopyWithin, 2);

// ES6 draft 05-05-15, section 22.2.3.7
function TypedArrayEvery(f, receiver) {
  if (!%IsTypedArray(this)) throw MakeTypeError(kNotTypedArray);

  var length = %_TypedArrayGetLength(this);

  return $innerArrayEvery(f, receiver, this, length);
}
%FunctionSetLength(TypedArrayEvery, 1);

// ES6 draft 08-24-14, section 22.2.3.12
function TypedArrayForEach(f, receiver) {
  if (!%IsTypedArray(this)) throw MakeTypeError(kNotTypedArray);

  var length = %_TypedArrayGetLength(this);

  $innerArrayForEach(f, receiver, this, length);
}
%FunctionSetLength(TypedArrayForEach, 1);

// ES6 draft 04-05-14 section 22.2.3.8
function TypedArrayFill(value, start, end) {
  if (!%IsTypedArray(this)) throw MakeTypeError(kNotTypedArray);

  var length = %_TypedArrayGetLength(this);

  return $innerArrayFill(value, start, end, this, length);
}
%FunctionSetLength(TypedArrayFill, 1);

// ES6 draft 07-15-13, section 22.2.3.9
function TypedArrayFilter(predicate, thisArg) {
  if (!%IsTypedArray(this)) throw MakeTypeError(kNotTypedArray);

  var length = %_TypedArrayGetLength(this);
  var array = $innerArrayFilter(predicate, thisArg, this, length);
  return ConstructTypedArrayLike(this, array);
}
%FunctionSetLength(TypedArrayFilter, 1);

// ES6 draft 07-15-13, section 22.2.3.10
function TypedArrayFind(predicate, thisArg) {
  if (!%IsTypedArray(this)) throw MakeTypeError(kNotTypedArray);

  var length = %_TypedArrayGetLength(this);

  return $innerArrayFind(predicate, thisArg, this, length);
}
%FunctionSetLength(TypedArrayFind, 1);

// ES6 draft 07-15-13, section 22.2.3.11
function TypedArrayFindIndex(predicate, thisArg) {
  if (!%IsTypedArray(this)) throw MakeTypeError(kNotTypedArray);

  var length = %_TypedArrayGetLength(this);

  return $innerArrayFindIndex(predicate, thisArg, this, length);
}
%FunctionSetLength(TypedArrayFindIndex, 1);


// ES6 draft 07-15-13, section 22.2.3.18
function TypedArrayMap(predicate, thisArg) {
  if (!%IsTypedArray(this)) throw MakeTypeError(kNotTypedArray);

  // TODO(littledan): Preallocate rather than making an intermediate
  // array, for better performance.
  var length = %_TypedArrayGetLength(this);
  var array = $innerArrayMap(predicate, thisArg, this, length);
  return ConstructTypedArrayLike(this, array);
}
%FunctionSetLength(TypedArrayMap, 1);


// ES6 draft 07-15-13, section 22.2.3.19
function TypedArrayReduce(callback, current) {
  if (!%IsTypedArray(this)) throw MakeTypeError(kNotTypedArray);

  var length = %_TypedArrayGetLength(this);
  return $innerArrayReduce(callback, current, this, length,
                           %_ArgumentsLength());
}
%FunctionSetLength(TypedArrayReduce, 1);


// ES6 draft 07-15-13, section 22.2.3.19
function TypedArrayReduceRight(callback, current) {
  if (!%IsTypedArray(this)) throw MakeTypeError(kNotTypedArray);

  var length = %_TypedArrayGetLength(this);
  return $innerArrayReduceRight(callback, current, this, length,
                                %_ArgumentsLength());
}
%FunctionSetLength(TypedArrayReduceRight, 1);


// ES6 draft 05-05-15, section 22.2.3.24
function TypedArraySome(f, receiver) {
  if (!%IsTypedArray(this)) throw MakeTypeError(kNotTypedArray);

  var length = %_TypedArrayGetLength(this);

  return $innerArraySome(f, receiver, this, length);
}
%FunctionSetLength(TypedArraySome, 1);


// ES6 draft 08-24-14, section 22.2.2.2
function TypedArrayOf() {
  var length = %_ArgumentsLength();
  var array = new this(length);
  for (var i = 0; i < length; i++) {
    array[i] = %_Arguments(i);
  }
  return array;
}

function ConstructTypedArray(constructor, array) {
  // TODO(littledan): This is an approximation of the spec, which requires
  // that only real TypedArray classes should be accepted (22.2.2.1.1)
  if (!IS_SPEC_OBJECT(constructor) || IS_UNDEFINED(constructor.prototype) ||
      !%HasOwnProperty(constructor.prototype, "BYTES_PER_ELEMENT")) {
    throw MakeTypeError(kNotTypedArray);
  }

  // TODO(littledan): The spec requires that, rather than directly calling
  // the constructor, a TypedArray is created with the proper proto and
  // underlying size and element size, and elements are put in one by one.
  // By contrast, this would allow subclasses to make a radically different
  // constructor with different semantics.
  return new constructor(array);
}

function TypedArrayFrom(source, mapfn, thisArg) {
  // TODO(littledan): Investigate if there is a receiver which could be
  // faster to accumulate on than Array, e.g., a TypedVector.
  var array = %_CallFunction(GlobalArray, source, mapfn, thisArg, $arrayFrom);
  return ConstructTypedArray(this, array);
}
%FunctionSetLength(TypedArrayFrom, 1);

// TODO(littledan): Fix the TypedArray proto chain (bug v8:4085).
macro EXTEND_TYPED_ARRAY(NAME)
  // Set up non-enumerable functions on the object.
  $installFunctions(GlobalNAME, DONT_ENUM | DONT_DELETE | READ_ONLY, [
    "from", TypedArrayFrom,
    "of", TypedArrayOf
  ]);

  // Set up non-enumerable functions on the prototype object.
  $installFunctions(GlobalNAME.prototype, DONT_ENUM, [
    "copyWithin", TypedArrayCopyWithin,
    "every", TypedArrayEvery,
    "fill", TypedArrayFill,
    "filter", TypedArrayFilter,
    "find", TypedArrayFind,
    "findIndex", TypedArrayFindIndex,
    "forEach", TypedArrayForEach,
    "map", TypedArrayMap,
    "reduce", TypedArrayReduce,
    "reduceRight", TypedArrayReduceRight,
    "some", TypedArraySome
  ]);
endmacro

TYPED_ARRAYS(EXTEND_TYPED_ARRAY)

})
