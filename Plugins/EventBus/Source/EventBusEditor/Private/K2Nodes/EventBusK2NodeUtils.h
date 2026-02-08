#pragma once

#include "CoreMinimal.h"

class UClass;
class UEdGraphPin;

namespace EventBusK2NodeUtils
{
	/**
	 * @brief Resolves best-known UObject class from a pin default or connected pin types.
	 */
	UClass* ResolveObjectClassFromPin(const UEdGraphPin* ObjectPin);
}

