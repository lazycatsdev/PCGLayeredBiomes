/*
 * Copyright (c) 2024 LazyCatsDev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


#pragma once

#include "CoreMinimal.h"
#include "PCGSettings.h"
#include "Elements/PCGPointProcessingElementBase.h"
#include "PCGGetAttributes.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, ClassGroup = (Biomes))
class PCGLAYEREDBIOMES_API UPCGGetAttributesSettings : public UPCGSettings
{
	GENERATED_BODY()

	//~Begin UPCGSettings interface
	#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const override { return FName(TEXT("Get Attributes")); }
	virtual FText GetDefaultNodeTitle() const override { return NSLOCTEXT("UPCGGetAttributesSettings", "NodeTitle", "Get Attributes"); }
	virtual FText GetNodeTooltipText() const override { return NSLOCTEXT("UPCGGetAttributesSettings", "NodeTooltip", "Get specified attributes as output pins"); }
	virtual EPCGSettingsType GetType() const override { return EPCGSettingsType::Metadata; }
	#endif

protected:
	virtual TArray<FPCGPinProperties> InputPinProperties() const override;
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override;

	virtual FPCGElementPtr CreateElement() const override;
	//~End UPCGSettings interface

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Biomes)
	TArray<FName> Attributes;
};

class PCGLAYEREDBIOMES_API FPCGGetAttributes final : public FPCGPointProcessingElementBase
{
protected:
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
};

/**
 * 
 */
UCLASS(BlueprintType, ClassGroup = (Biomes))
class PCGLAYEREDBIOMES_API UPCGGetAllAttributesFromSettings : public UPCGSettings
{
	GENERATED_BODY()

	//~Begin UPCGSettings interface
	#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const override { return FName(TEXT("GetAllAttributesFrom")); }
	virtual FText GetDefaultNodeTitle() const override { return NSLOCTEXT("UPCGGetAllAttributesFromSettings", "NodeTitle", "Get All Attributes From"); }
	virtual EPCGSettingsType GetType() const override { return EPCGSettingsType::Metadata; }
	#endif

protected:
	virtual TArray<FPCGPinProperties> InputPinProperties() const override;
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override;

	virtual FPCGElementPtr CreateElement() const override;
	//~End UPCGSettings interface

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Biomes, meta=(PCG_Overridable))
	int DataIndex = 0;
};

class PCGLAYEREDBIOMES_API FPCGGetAllAttributesFrom final : public FPCGPointProcessingElementBase
{
protected:
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
};

