/**
 * @file Util.h
 * @brief Generic validity helpers and guard macros for EventBus call-sites.
 *
 * This header is intentionally plugin-generic and must not include game-specific types.
 */
#pragma once

#include "CoreMinimal.h"

/**
 * Standard type traits are intentionally used.
 *
 * Rationale:
 * - UE 5.5 deprecates several legacy trait aliases and points toward std equivalents.
 * - This file relies on std::void_t, std::enable_if, std::is_pointer, std::remove_pointer,
 *   and std::remove_cv_t for C++17/C++20-compatible validity helpers.
 *
 * UE deprecation context:
 * - UnrealTypeTraits.h deprecates TIsSame with:
 *   "TIsSame has been deprecated, please use std::is_same instead."
 * - Build warnings in UE 5.5 also deprecate TRemoveCV in favor of std::remove_cv_t.
 */
#include <type_traits>

#if __cplusplus >= 202002L
/**
 * C++20 concepts are intentionally used for the C++20 path.
 *
 * Rationale:
 * - HasIsValid/HasGet concepts provide clearer constraints and diagnostics than trait-only SFINAE.
 * - UE 5.5 does not expose equivalent UE-native concepts for these checks.
 */
#include <concepts>
#endif

namespace Nfrrlib
{
	/**
	 * @brief Validity helpers that normalize checks across raw pointers and UE pointer wrappers.
	 *
	 * - If the type exposes IsValid(), use it.
	 * - Else if it exposes Get(), validate the raw UObject pointer.
	 * This keeps NFL_TRY_PTR readable and consistent across pointer kinds.
	 *
	 * Provides both C++20 (concepts) and C++17 (SFINAE) implementations for compatibility.
	 */

#if __cplusplus >= 202002L
	// ===========================
	// C++20 Concept-Based Implementation
	// ===========================

	template <typename T>
	concept HasIsValid = requires(const T& Ptr)
	{
		{ Ptr.IsValid() } -> std::convertible_to<bool>;
	};

	template <typename T>
	concept HasGet = requires(const T& Ptr)
	{
		Ptr.Get();
	};

	template <typename T>
	FORCEINLINE bool IsValid(const T& Ptr)
		requires HasIsValid<T>
	{
		// UE smart pointers (TWeakObjectPtr, TSoftObjectPtr, etc.).
		return Ptr.IsValid();
	}

	template <typename T>
	FORCEINLINE bool IsValid(const T& Ptr)
		requires (!HasIsValid<T> && HasGet<T>)
	{
		// Pointer wrappers that expose Get() (e.g., TObjectPtr).
		return ::IsValid(Ptr.Get());
	}

	template <typename T>
	FORCEINLINE bool IsValid(const T* Ptr)
		requires TIsDerivedFrom<T, UObject>::IsDerived
	{
		// Raw UObject pointer.
		return ::IsValid(Ptr);
	}
	
	template <typename T>
	FORCEINLINE bool IsValid(const T* Ptr)
		requires (!TIsDerivedFrom<T, UObject>::IsDerived)
	{
		// Raw pointer to non-UObject type (e.g., enum, struct).
		return Ptr != nullptr;
	}

#else
	// ===========================
	// C++17 SFINAE-Based Implementation
	// ===========================

	// Helper to detect if type has IsValid() method
	template <typename T, typename = void>
	struct THasIsValid : std::false_type {};

	template <typename T>
	struct THasIsValid<T, std::void_t<decltype(std::declval<const T&>().IsValid())>> : std::true_type {};

	// Helper to detect if type has Get() method
	template <typename T, typename = void>
	struct THasGet : std::false_type {};

	template <typename T>
	struct THasGet<T, std::void_t<decltype(std::declval<const T&>().Get())>> : std::true_type {};

	// Overload 1: Types with IsValid() method (e.g., TWeakObjectPtr, TSoftObjectPtr)
	template <typename T>
	FORCEINLINE typename std::enable_if<THasIsValid<T>::value, bool>::type
	IsValid(const T& Ptr)
	{
		return Ptr.IsValid();
	}

	// Overload 2: Types with Get() but no IsValid() (e.g., TObjectPtr)
	template <typename T>
	FORCEINLINE typename std::enable_if<!THasIsValid<T>::value && THasGet<T>::value, bool>::type
	IsValid(const T& Ptr)
	{
		return ::IsValid(Ptr.Get());
	}

	// Overload 3: Raw UObject-derived pointers
	template <typename T>
	FORCEINLINE typename std::enable_if<
		std::is_pointer<T>::value && 
		TIsDerivedFrom<typename std::remove_pointer<T>::type, UObject>::IsDerived, 
		bool>::type
	IsValid(T Ptr)
	{
		return ::IsValid(Ptr);
	}

	// Overload 4: Raw non-UObject pointers
	template <typename T>
	FORCEINLINE typename std::enable_if<
		std::is_pointer<T>::value && 
		!TIsDerivedFrom<typename std::remove_pointer<T>::type, UObject>::IsDerived, 
		bool>::type
	IsValid(T Ptr)
	{
		return Ptr != nullptr;
	}

#endif	// __cplusplus >= 202002L

}	// namespace Nfrrlib

#ifndef NFL_MACROS
#define NFL_MACROS

/** No-op action helper for TRY macros. */
#define NFL_TRY_NOP do { } while (0)

/**
 * @def NFL_TRY_PTR
 * @brief Validates a pointer/wrapper and executes FailureAction on invalid input.
 */
#define NFL_TRY_PTR(Ptr, FailureAction) \
	do \
	{ \
		if (!Nfrrlib::IsValid(Ptr)) \
		{ \
			UE_LOG(LogTemp, Warning, TEXT("%hs aborted: invalid pointer " #Ptr), __FUNCTION__); \
			FailureAction; \
		} \
} while (0)

/**
 * @def NFL_TRY_GET_WORLD_2
 * @brief Loads `UWorld*` into `OutWorld` and validates it.
 */
#define NFL_TRY_GET_WORLD_2(OutWorld, FailureAction) \
UWorld* OutWorld = GetWorld(); \
NFL_TRY_PTR(OutWorld, FailureAction)

/** @brief Overload with no failure action. */
#define NFL_TRY_GET_WORLD_1(OutWorld) \
NFL_TRY_GET_WORLD_2(OutWorld, NFL_TRY_NOP)

/** Internal vararg selector helper (1-2 parameters). */
#define NFL_SELECT_TRY_GET_MACRO_1_2(_1, _2, NAME, ...) NAME
/** Internal vararg selector helper (2-3 parameters). */
#define NFL_SELECT_TRY_GET_MACRO_2_3(_1, _2, _3, NAME, ...) NAME

/** @brief Public entry point for world retrieval (`NFL_TRY_GET_WORLD_1` or `_2`). */
#define NFL_TRY_GET_WORLD(...) \
NFL_SELECT_TRY_GET_MACRO_1_2(__VA_ARGS__, NFL_TRY_GET_WORLD_2, NFL_TRY_GET_WORLD_1)(__VA_ARGS__)

#endif // NFL_MACROS
