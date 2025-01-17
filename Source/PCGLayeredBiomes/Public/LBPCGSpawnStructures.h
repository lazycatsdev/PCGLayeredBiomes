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
#include "Engine/DataAsset.h"
#include "Components/StaticMeshComponent.h"
#include "LBPCGSpawnStructures.generated.h"

struct FPCGCrc;

UCLASS(Abstract, Const, Blueprintable, DefaultToInstanced, EditInlineNew, CollapseCategories, ClassGroup=(Biomes))
class ULBBiomesInstanceUserData : public UObject
{
	GENERATED_BODY()
};

/**
 * 
 */
USTRUCT(BlueprintType, Category=Biomes)
struct PCGLAYEREDBIOMES_API FLBPCGSpawnInfo
{
	GENERATED_BODY()

	FPCGCrc ComputeCrc() const;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category=Biomes)
	TSoftObjectPtr<UStaticMesh> Mesh;

	/**
	 * Any additional information for that InstancedMesh
	 */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Instanced, Category=Biomes)
	TObjectPtr<ULBBiomesInstanceUserData> UserData;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category=Biomes, meta=(ClampMin=1, UIMin=1))
	int Weight = 1;
};

USTRUCT(BlueprintType, Category=Biomes)
struct PCGLAYEREDBIOMES_API FLBPCGSpawnSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category=Biomes)
	FString Name;

	UPROPERTY(EditAnywhere, Category=Biomes)
	TArray<FLBPCGSpawnInfo> Actors;
};

UCLASS(ClassGroup=(Biomes))
class PCGLAYEREDBIOMES_API ULBPCGSpawnPreset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	FPCGCrc ComputeCrc();
	bool HasUserData() const;

	UPROPERTY(EditAnywhere, Category=Biomes, meta=(TitleProperty="Name"))
	TArray<FLBPCGSpawnSet> Sets;
};
