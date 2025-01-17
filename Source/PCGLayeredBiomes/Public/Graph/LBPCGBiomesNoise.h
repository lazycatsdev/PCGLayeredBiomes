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
#include "Metadata/PCGAttributePropertySelector.h"

#include "LBPCGBiomesNoise.generated.h"

namespace PCGBiomesNoise
{
	struct FLocalCoordinates2D
	{
		// coordinates to sample for the repeating edges (not 0 to 1, is scaled according to the settings scale)
		double X0;
		double Y0;
		double X1;
		double Y1;

		// how much to interpolate between the corners
		double FracX;
		double FracY;
	};

	PCGLAYEREDBIOMES_API FLocalCoordinates2D CalcLocalCoordinates2D(const FBox& ActorLocalBox, const FTransform& ActorTransformInverse, FVector2D Scale, const FVector& Position);
	PCGLAYEREDBIOMES_API double CalcEdgeBlendAmount2D(const FLocalCoordinates2D& LocalCoords, double EdgeBlendDistance);
}

UCLASS(BlueprintType)
class PCGLAYEREDBIOMES_API ULBPCGBiomesNoiseSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
	ULBPCGBiomesNoiseSettings();

	//~Begin UPCGSettings interface
#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const override { return FName(TEXT("BiomesNoise")); }
	virtual FText GetDefaultNodeTitle() const override { return NSLOCTEXT("PCGNoiseSettings", "NodeTitle", "Biomes Noise"); }
	virtual EPCGSettingsType GetType() const override { return EPCGSettingsType::Spatial; }
#endif

protected:
	virtual TArray<FPCGPinProperties> InputPinProperties() const override;
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override;

	virtual FPCGElementPtr CreateElement() const override;
	//~End UPCGSettings interface

public:

	// this is how many times the fractal method recurses. A higher number will mean more detail
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (ClampMin = "1", ClampMax = "100", EditConditionHides, PCG_Overridable))
	int32 Iterations = 4;

	// if true, will generate results that tile along the bounding box size of the 
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
	bool bTiling = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
	float Brightness = 0.0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
	float Contrast = 1.0;

	// The output attribute name to write, if not 'None'
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings)
	FPCGAttributePropertyOutputNoSourceSelector ValueTarget;

	// Adds a random amount of offset up to this amount
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
	FVector RandomOffset = FVector(100000.0);

	// this will apply a transform to the points before calculating noise
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "!bTiling", EditConditionHides, PCG_Overridable))
	float Scale = 1.0f;
};

class PCGLAYEREDBIOMES_API FLBPCGBiomesNoise : public FPCGPointProcessingElementBase
{
protected:
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
};
