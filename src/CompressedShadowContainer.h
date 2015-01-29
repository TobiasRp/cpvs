#ifndef COMPRESSED_SHADOW_CONTAINER
#define COMPRESSED_SHADOW_CONTAINER

#include "cpvs.h"
#include "CompressedShadow.h"
#include "Buffer.h"
#include "ShaderProgram.h"

class Texture2D;

/** Contains one or more CompressedShadows, which can be added sequentially to the container.
 *
 * The container can be moved to the GPU, thereby freeing all data on the CPU and moving them to the GPU.
 * After this all operations working on the CPU representation become unusable.
 *
 * After the container has been moved or copied to the GPU the precomputed shadows can be evaluated.
 */
class CompressedShadowContainer {
public:
	/** Creates a container with a fixed length in one dimension of the 3D container. */
	CompressedShadowContainer(uint length)
		: m_length(length)
	{
		m_data.resize(length * length * length);
	}

	/** Creates a container with a length of 1 and initializes it with the given precomputed shadow. */
	CompressedShadowContainer(unique_ptr<CompressedShadow> shadow)
		: m_length(1)
	{
		m_data.push_back(std::move(shadow));
	}

	inline void set(unique_ptr<CompressedShadow> shadow, uint x, uint y, uint z) {
		assert(x < m_length && y < m_length && z < m_length);

		const uint lengthSquared = m_length * m_length;
		m_data[z * lengthSquared + y * m_length + x] = std::move(shadow);
	}

	const CompressedShadow* get(uint x, uint y, uint z) const {
		assert(x < m_length && y < m_length && z < m_length);

		const uint lengthSquared = m_length * m_length;
		return m_data[z * lengthSquared + y * m_length + x].get();
	}

	/**
	 * Calculates the visibility/shadow of every world-space position in the given texture.
	 * The result is a 2-dimensional texture of visibility values.
	 */
	void evaluate(const Texture2D* positionsWS, const mat4& lightViewProj, Texture2D* visibilities);

	/** Frees all dynamically allocated memory on the CPU. */
	inline void freeOnCPU() {
		// Use the 'swap trick' to free all dynamic memory
		std::vector<unique_ptr<CompressedShadow>> tmp;
		m_data.swap(tmp);
	}

	/** Copy all shadows to the device memory. */
	void copyToGPU();

	/** Combines copyToGPU and freeOnCPU, i.e. copies the data to the GPU and free's it on the CPU */
	inline void moveToGPU() {
		copyToGPU();
		freeOnCPU();
	}

private:
	void initShader();

	vector<uint> createTopLevelGrid();

	vector<uint> combineDAGs();

private:
	uint m_length;
	vector<unique_ptr<CompressedShadow>> m_data;

	unique_ptr<SSBO> m_deviceDag;
	unique_ptr<SSBO> m_deviceGrid;

	unique_ptr<ShaderProgram> m_traverseCS;
};

#endif
