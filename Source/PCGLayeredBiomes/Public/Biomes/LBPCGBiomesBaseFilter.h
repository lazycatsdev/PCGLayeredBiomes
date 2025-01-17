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
#include "UObject/Object.h"
#include "LBPCGBiomesBaseFilter.generated.h"

/**
 * 
 */
UCLASS(Abstract, Const, DefaultToInstanced, EditInlineNew, Blueprintable, CollapseCategories)
class PCGLAYEREDBIOMES_API ULBPCGBiomesBaseFilter : public UObject
{
	GENERATED_BODY()

public:

	/**
	 * Should return true for a point if this point belongs to the biome.
	 * @param Point Point data from PCG. Contains default points data and all the data from landscape.  
	 * @param Metadata Reference to metadata where all values of attributes are stored.
	 * @return True if a point belongs to the biome and false otherwise. 
	 */
	UFUNCTION(BlueprintImplementableEvent, Category=Biomes)
	bool Filter(const FPCGPoint& Point, const UPCGMetadata* Metadata) const;
};
