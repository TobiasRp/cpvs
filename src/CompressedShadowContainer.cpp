#include "CompressedShadowContainer.h"
#include "Texture.h"

#include <iostream>
using namespace std;

static const uint GRID_CELL_SHADOWED = 0xFFFFFFF;
static const uint GRID_CELL_VISIBLE = 0xFFFFFFE;

// Is called when the DAG is copied to the GPU
void CompressedShadowContainer::initShader() {
	m_traverseCS = make_unique<ShaderProgram>();
	try {
		m_traverseCS->addShaderFromFile(GL_COMPUTE_SHADER, "../shader/traverse.cs");
		m_traverseCS->link();
	} catch(ShaderException& exc) {
		cout << exc.where() << " - " << exc.what() << endl;
		std::terminate();
	}

	m_traverseCS->bind();
	m_traverseCS->addUniform("lightViewProj");
	m_traverseCS->addUniform("width");
	m_traverseCS->addUniform("height");
	m_traverseCS->addUniform("dag_levels");
	m_traverseCS->addUniform("grid_levels");
}

void CompressedShadowContainer::copyToGPU() {
	assert(m_data.size() > 0);
	initShader();

	m_deviceDag = make_unique<SSBO>(combineDAGs(), GL_STATIC_READ);

	m_deviceGrid = make_unique<SSBO>(createTopLevelGrid(), GL_STATIC_READ);

	// number of levels has to be the same in every DAG
	glUniform1i((*m_traverseCS)["dag_levels"], m_data[0]->getNumLevels());

	const uint gridLevels = log8(m_data.size());
	glUniform1i((*m_traverseCS)["grid_levels"], gridLevels);
}

vector<uint> CompressedShadowContainer::combineDAGs() {
	if (m_data.size() == 1)
		return m_data[0]->getDAG();

	vector<uint> combinedDAG;
	for (auto& csPtr : m_data) {
		combinedDAG.insert(combinedDAG.end(), csPtr->getDAG().begin(), csPtr->getDAG().end());
	}
	return combinedDAG;
}

vector<uint> CompressedShadowContainer::createTopLevelGrid() {
	vector<uint> grid;
	grid.reserve(m_data.size());

	uint offset = 0;
	for (auto& csPtr : m_data) {
		auto visibility = csPtr->getTotalVisibility();

		// Store a special value for shadow/visible if the entire grid cell is in shadow/visible.
		// otherwise store the offset to the DAG
		if (visibility == CompressedShadow::SHADOW)
			grid.push_back(GRID_CELL_SHADOWED);
		else if (visibility == CompressedShadow::VISIBLE)
			grid.push_back(GRID_CELL_VISIBLE);
		else
			grid.push_back(offset);

		offset += csPtr->getDAG().size();
	}
	return grid;
}

void CompressedShadowContainer::evaluate(const Texture2D* positionsWS, const mat4& lightViewProj,
		Texture2D* visibilities) {
	GL_CHECK_ERROR("traverse - begin");
	assert(m_traverseCS != nullptr);
	m_traverseCS->bind();

	// Bind WS positions
	positionsWS->bindImageAt(0, GL_READ_ONLY);

	// Bind image for results
	visibilities->bindImageAt(1, GL_WRITE_ONLY);

	// Bind dag and grid
	m_deviceDag->bindAt(2);
	m_deviceGrid->bindAt(3);

	glUniformMatrix4fv((*m_traverseCS)["lightViewProj"], 1, GL_FALSE, glm::value_ptr(lightViewProj));

	const GLuint width = positionsWS->getWidth();
	const GLuint height = positionsWS->getHeight();
	glUniform1ui((*m_traverseCS)["width"], width);
	glUniform1ui((*m_traverseCS)["height"], height);

	// Now calculate work group size and dispatch!
	const GLuint localSize = 32;
	const GLuint numGroupsX = ceil(width / static_cast<float>(localSize));
	const GLuint numGroupsY = ceil(height / static_cast<float>(localSize));
	glDispatchCompute(numGroupsX, numGroupsY, 1);

	m_traverseCS->release();
	GL_CHECK_ERROR("traverse - end");
}
