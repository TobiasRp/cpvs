#include "CompressedShadow.h"
#include "MinMaxHierarchy.h"
#include "ShadowMap.h"


CompressedShadow::CompressedShadow(const MinMaxHierarchy& minMax) {

}

unique_ptr<CompressedShadow> CompressedShadow::create(const MinMaxHierarchy& minMax) {
	return unique_ptr<CompressedShadow>(new CompressedShadow(minMax));
}

unique_ptr<CompressedShadow> CompressedShadow::create(const ShadowMap& shadowMap) {
	MinMaxHierarchy minMax(shadowMap.createImageF());
	return create(minMax);
}
