#include "ShadowApplication.h"

#include "Terrain.h"

#include <ituGL/asset/TextureCubemapLoader.h>
#include <ituGL/asset/ShaderLoader.h>
#include <ituGL/asset/ModelLoader.h>

#include <ituGL/camera/Camera.h>
#include <ituGL/scene/SceneCamera.h>

#include <ituGL/lighting/DirectionalLight.h>
#include <ituGL/lighting/PointLight.h>
#include <ituGL/scene/SceneLight.h>

#include <ituGL/shader/ShaderUniformCollection.h>
#include <ituGL/shader/Material.h>
#include <ituGL/geometry/Model.h>
#include <ituGL/scene/SceneModel.h>

#include <ituGL/renderer/SkyboxRenderPass.h>
#include <ituGL/renderer/GBufferRenderPass.h>
#include <ituGL/renderer/DeferredRenderPass.h>
#include <ituGL/renderer/ShadowMapRenderPass.h>
#include <ituGL/renderer/PostFXRenderPass.h>
#include <ituGL/renderer/DebugRenderPass.h>
#include <ituGL/scene/RendererSceneVisitor.h>

#include <ituGL/scene/Transform.h>
#include <ituGL/scene/ImGuiSceneVisitor.h>
#include <imgui.h>

#include <stb_image.h>
#include <iostream>
#include <glm/gtx/string_cast.hpp>

#define N_OF_CASCADES 3

ShadowApplication::ShadowApplication()
	: Application(1024, 1024, "Shadow Scene Viewer demo")
	, m_renderer(GetDevice())
	, m_sceneFramebuffer(std::make_shared<FramebufferObject>())
	, m_exposure(0.41f)
	, m_contrast(1.0f)
	, m_hueShift(0.0f)
	, m_saturation(1.0f)
	, m_colorFilter(1.0f)
	, m_blurIterations(1)
	, m_bloomRange(1.0f, 2.0f)
	, m_bloomIntensity(1.0f)
	, m_terrainColor(1.0f)
{
}

void ShadowApplication::Initialize()
{
	Application::Initialize();

	// Initialize DearImGUI
	m_imGui.Initialize(GetMainWindow());

	InitializeCamera();
	InitializeLights();
	InitializeMaterials();
	InitializeModels();
	InitializeRenderer();
}

void ShadowApplication::Update()
{
	Application::Update();

	// Update camera controller
	m_cameraController.Update(GetMainWindow(), GetDeltaTime());

	// Add the scene nodes to the renderer
	RendererSceneVisitor rendererSceneVisitor(m_renderer);
	m_scene.AcceptVisitor(rendererSceneVisitor);


	if (GetMainWindow().IsKeyPressed(GLFW_KEY_T))
	{
		static_cast<ShadowMapRenderPass*>(m_renderer.GetRenderPass(m_shadowPassIndex))->shouldFreeze = true;
	}
	if (GetMainWindow().IsKeyPressed(GLFW_KEY_U))
	{
		static_cast<ShadowMapRenderPass*>(m_renderer.GetRenderPass(m_shadowPassIndex))->shouldFreeze = false;
	}
}

void ShadowApplication::Render()
{
	Application::Render();

	GetDevice().Clear(true, Color(0.0f, 0.0f, 0.0f, 1.0f), true, 1.0f);

	m_renderer.Render();
	RenderGUI();
}

void ShadowApplication::Cleanup()
{
	m_imGui.Cleanup();
	Application::Cleanup();
}

void ShadowApplication::InitializeCamera()
{
	std::shared_ptr<Camera> camera = std::make_shared<Camera>();
	camera->SetViewMatrix(glm::vec3(-2, 1, -2), glm::vec3(0, 0.5f, 0), glm::vec3(0, 1, 0));
	camera->SetPerspectiveProjectionMatrix(2.0f, 1.0f, 0.1f, 150.0f);
	std::shared_ptr<SceneCamera> sceneCamera = std::make_shared<SceneCamera>("camera", camera);
	m_scene.AddSceneNode(sceneCamera);
	m_cameraController.SetCamera(sceneCamera);
}

void ShadowApplication::InitializeLights()
{
	// Create a directional light and add it to the scene
	std::shared_ptr<DirectionalLight> directionalLight = std::make_shared<DirectionalLight>();
	directionalLight->SetDirection(glm::vec3(0.29f, -0.5f, 1.4f)); // It will be normalized inside the function
	directionalLight->SetIntensity(13.3f);
	m_scene.AddSceneNode(std::make_shared<SceneLight>("directional light", directionalLight));

	m_mainLight = directionalLight;
}

void ShadowApplication::InitializeMaterials()
{
	// Shadow map material
	{
		// Load and build shader
		std::vector<const char*> vertexShaderPaths;
		vertexShaderPaths.push_back("shaders/version330.glsl");
		vertexShaderPaths.push_back("shaders/csm.vert");
		Shader vertexShader = ShaderLoader(Shader::VertexShader).Load(vertexShaderPaths);

		std::vector<const char*> geometryShaderPaths;
		geometryShaderPaths.push_back("shaders/version330.glsl");
		geometryShaderPaths.push_back("shaders/csm.geom");
		Shader geometryShader = ShaderLoader(Shader::GeometryShader).Load(geometryShaderPaths);

		std::vector<const char*> fragmentShaderPaths;
		fragmentShaderPaths.push_back("shaders/version330.glsl");
		fragmentShaderPaths.push_back("shaders/renderer/empty.frag");
		Shader fragmentShader = ShaderLoader(Shader::FragmentShader).Load(fragmentShaderPaths);

		std::shared_ptr<ShaderProgram> shaderProgramPtr = std::make_shared<ShaderProgram>();
		shaderProgramPtr->Build(vertexShader, fragmentShader, geometryShader);

		// Get transform related uniform locations
		ShaderProgram::Location worldViewProjMatrixLocation = shaderProgramPtr->GetUniformLocation("WorldViewProjMatrix");

		// Register shader with renderer
		m_renderer.RegisterShaderProgram(shaderProgramPtr,
			[=](const ShaderProgram& shaderProgram, const glm::mat4& worldMatrix, const Camera& camera, bool cameraChanged)
			{
				shaderProgram.SetUniform(worldViewProjMatrixLocation, camera.GetViewProjectionMatrix() * worldMatrix);
			},
			nullptr
		);

		// Filter out uniforms that are not material properties
		ShaderUniformCollection::NameSet filteredUniforms;
		filteredUniforms.insert("WorldViewProjMatrix");

		// Create material
		m_shadowMapMaterial = std::make_shared<Material>(shaderProgramPtr, filteredUniforms);
		m_shadowMapMaterial->SetCullMode(Material::CullMode::Front);
	}

	// G-buffer material
	{
		// Load and build shader
		std::vector<const char*> vertexShaderPaths;
		vertexShaderPaths.push_back("shaders/version330.glsl");
		vertexShaderPaths.push_back("shaders/default.vert");
		Shader vertexShader = ShaderLoader(Shader::VertexShader).Load(vertexShaderPaths);

		std::vector<const char*> fragmentShaderPaths;
		fragmentShaderPaths.push_back("shaders/version330.glsl");
		fragmentShaderPaths.push_back("shaders/utils.glsl");
		fragmentShaderPaths.push_back("shaders/default.frag");
		Shader fragmentShader = ShaderLoader(Shader::FragmentShader).Load(fragmentShaderPaths);

		std::shared_ptr<ShaderProgram> shaderProgramPtr = std::make_shared<ShaderProgram>();
		shaderProgramPtr->Build(vertexShader, fragmentShader);

		// Get transform related uniform locations
		ShaderProgram::Location worldViewMatrixLocation = shaderProgramPtr->GetUniformLocation("WorldViewMatrix");
		ShaderProgram::Location worldViewProjMatrixLocation = shaderProgramPtr->GetUniformLocation("WorldViewProjMatrix");

		// Register shader with renderer
		m_renderer.RegisterShaderProgram(shaderProgramPtr,
			[=](const ShaderProgram& shaderProgram, const glm::mat4& worldMatrix, const Camera& camera, bool cameraChanged)
			{
				shaderProgram.SetUniform(worldViewMatrixLocation, camera.GetViewMatrix() * worldMatrix);
				shaderProgram.SetUniform(worldViewProjMatrixLocation, camera.GetViewProjectionMatrix() * worldMatrix);
			},
			nullptr
		);

		// Filter out uniforms that are not material properties
		ShaderUniformCollection::NameSet filteredUniforms;
		filteredUniforms.insert("WorldViewMatrix");
		filteredUniforms.insert("WorldViewProjMatrix");

		// Create material
		m_defaultMaterial = std::make_shared<Material>(shaderProgramPtr, filteredUniforms);
		m_defaultMaterial->SetUniformValue("Color", glm::vec3(1.0f, 0.0f, 1.0f));
		// Use default textures
		m_default_colorTexture = LoadTexture("textures/Default_Albedo.jpg");
		m_defaultMaterial->SetUniformValue("ColorTexture", m_default_colorTexture);
		m_default_normalTexture = LoadTexture("textures/Default_NormalMap.png");
		m_defaultMaterial->SetUniformValue("NormalTexture", m_default_normalTexture);
		m_default_specularTexture = LoadTexture("textures/Default_ARM.png");
		m_defaultMaterial->SetUniformValue("SpecularTexture", m_default_specularTexture);
	}

	// Deferred material
	{
		std::vector<const char*> vertexShaderPaths;
		vertexShaderPaths.push_back("shaders/version330.glsl");
		vertexShaderPaths.push_back("shaders/renderer/deferred.vert");
		Shader vertexShader = ShaderLoader(Shader::VertexShader).Load(vertexShaderPaths);

		std::vector<const char*> fragmentShaderPaths;
		fragmentShaderPaths.push_back("shaders/version330.glsl");
		fragmentShaderPaths.push_back("shaders/utils.glsl");
		fragmentShaderPaths.push_back("shaders/lambert-ggx.glsl");
		fragmentShaderPaths.push_back("shaders/lighting.glsl");
		fragmentShaderPaths.push_back("shaders/renderer/deferred.frag");
		Shader fragmentShader = ShaderLoader(Shader::FragmentShader).Load(fragmentShaderPaths);

		std::shared_ptr<ShaderProgram> shaderProgramPtr = std::make_shared<ShaderProgram>();
		shaderProgramPtr->Build(vertexShader, fragmentShader);

		// Filter out uniforms that are not material properties
		ShaderUniformCollection::NameSet filteredUniforms;
		filteredUniforms.insert("InvViewMatrix");
		filteredUniforms.insert("InvProjMatrix");
		filteredUniforms.insert("WorldViewProjMatrix");
		filteredUniforms.insert("LightIndirect");
		filteredUniforms.insert("LightColor");
		filteredUniforms.insert("LightPosition");
		filteredUniforms.insert("LightDirection");
		filteredUniforms.insert("LightAttenuation");

		// Get transform related uniform locations
		ShaderProgram::Location invViewMatrixLocation = shaderProgramPtr->GetUniformLocation("InvViewMatrix");
		ShaderProgram::Location invProjMatrixLocation = shaderProgramPtr->GetUniformLocation("InvProjMatrix");
		ShaderProgram::Location worldViewProjMatrixLocation = shaderProgramPtr->GetUniformLocation("WorldViewProjMatrix");

		// Register shader with renderer
		m_renderer.RegisterShaderProgram(shaderProgramPtr,
			[=](const ShaderProgram& shaderProgram, const glm::mat4& worldMatrix, const Camera& camera, bool cameraChanged)
			{
				if (cameraChanged)
				{
					shaderProgram.SetUniform(invViewMatrixLocation, glm::inverse(camera.GetViewMatrix()));
					shaderProgram.SetUniform(invProjMatrixLocation, glm::inverse(camera.GetProjectionMatrix()));
				}
				shaderProgram.SetUniform(worldViewProjMatrixLocation, camera.GetViewProjectionMatrix() * worldMatrix);
			},
			m_renderer.GetDefaultUpdateLightsFunction(*shaderProgramPtr)
		);

		// Create material
		m_deferredMaterial = std::make_shared<Material>(shaderProgramPtr, filteredUniforms);
	}
}

void ShadowApplication::InitializeModels()
{
	m_skyboxTexture = TextureCubemapLoader::LoadTextureShared("models/skybox/StandardCubeMap.hdr", TextureObject::FormatRGB, TextureObject::InternalFormatRGB16F);

	m_skyboxTexture->Bind();
	float maxLod;
	m_skyboxTexture->GetParameter(TextureObject::ParameterFloat::MaxLod, maxLod);
	TextureCubemapObject::Unbind();

	// Set the environment texture on the deferred material
	m_deferredMaterial->SetUniformValue("EnvironmentTexture", m_skyboxTexture);
	m_deferredMaterial->SetUniformValue("EnvironmentMaxLod", maxLod);

	// Configure loader
	ModelLoader loader(m_defaultMaterial);

	// Create a new material copy for each submaterial
	loader.SetCreateMaterials(true);

	// Flip vertically textures loaded by the model loader
	loader.GetTexture2DLoader().SetFlipVertical(true);

	// Link vertex properties to attributes
	loader.SetMaterialAttribute(VertexAttribute::Semantic::Position, "VertexPosition");
	loader.SetMaterialAttribute(VertexAttribute::Semantic::Normal, "VertexNormal");
	loader.SetMaterialAttribute(VertexAttribute::Semantic::Tangent, "VertexTangent");
	loader.SetMaterialAttribute(VertexAttribute::Semantic::Bitangent, "VertexBitangent");
	loader.SetMaterialAttribute(VertexAttribute::Semantic::TexCoord0, "VertexTexCoord");

	// Link material properties to uniforms
	loader.SetMaterialProperty(ModelLoader::MaterialProperty::DiffuseColor, "Color");
	loader.SetMaterialProperty(ModelLoader::MaterialProperty::DiffuseTexture, "ColorTexture");
	loader.SetMaterialProperty(ModelLoader::MaterialProperty::NormalTexture, "NormalTexture");
	loader.SetMaterialProperty(ModelLoader::MaterialProperty::SpecularTexture, "SpecularTexture");
	
	// Generate terrain model
	std::shared_ptr<Mesh> terrainMesh = std::make_shared<Mesh>();
	float terrainSize = 150.0f;
	float terrainHeight = 50.0f;
	unsigned int terrainGridSize = 256u;
	float indexMultiplier = terrainSize / static_cast<float>(terrainGridSize - 1);
	m_heightmap = Terrain::CreateTerrainMesh(terrainMesh, terrainGridSize, terrainGridSize, terrainHeight, terrainSize);
	std::shared_ptr<Model> terrainModel = std::make_shared<Model>(terrainMesh);
	// Add AABB info and add to scene
	glm::vec3 terrainAABBMax = glm::vec3(terrainSize, terrainHeight * 0.5f, terrainSize);
	glm::vec3 terrainAABBMin = glm::vec3(0.0f, -terrainHeight * 0.5f, 0.0f);
	std::shared_ptr<SceneModel> terrainSceneModel = std::make_shared<SceneModel>("terrain", terrainModel, terrainAABBMin, terrainAABBMax);
	m_scene.AddSceneNode(terrainSceneModel);
	// Add material to terrain
	m_terrainMaterial = std::make_shared<Material>(*m_defaultMaterial);
	CreateTerrainMaterial(m_terrainMaterial);
	terrainModel.get()->AddMaterial(m_terrainMaterial);

	// Load tree model
	std::shared_ptr<Model> treeModel = loader.LoadShared("models/myTree/Tree.obj");
	glm::vec3 treeAABBExtents = glm::vec3(1.5f, 1.5f, 2.0f);
	glm::vec3 treeAABBMin;
	glm::vec3 treeAABBMax;
	// Generate random distribution of trees
	int distance = 25;
	for (unsigned int x = 0u; x < terrainGridSize / distance; ++x) {
		for (unsigned int y = 0u; y < terrainGridSize / distance; ++y) {
			// Get name and position
			std::string name("tree ");
			name += std::to_string(y * distance + x);
			glm::ivec2 treeCoords(distance * x + distance * GetRandomRange(-0.4f, 0.4f), distance * y + distance * GetRandomRange(-0.4f, 0.4f));
			float height = m_heightmap[treeCoords.y * (terrainGridSize + 1) + treeCoords.x];
			glm::vec3 position(treeCoords.x * indexMultiplier, height, treeCoords.y * indexMultiplier);
			//
			treeAABBMin = position - treeAABBExtents;
			treeAABBMax = position + treeAABBExtents;
			std::shared_ptr<SceneModel> sceneModel = std::make_shared<SceneModel>(name, treeModel, treeAABBMin, treeAABBMax);
			//
			sceneModel->GetTransform()->SetTranslation(position);
			m_scene.AddSceneNode(sceneModel);
		}
	}
}

void ShadowApplication::InitializeFramebuffers()
{
	int width, height;
	GetMainWindow().GetDimensions(width, height);

	// Scene Texture
	m_sceneTexture = std::make_shared<Texture2DObject>();
	m_sceneTexture->Bind();
	m_sceneTexture->SetImage(0, width, height, TextureObject::FormatRGBA, TextureObject::InternalFormat::InternalFormatRGBA16F);
	m_sceneTexture->SetParameter(TextureObject::ParameterEnum::MinFilter, GL_LINEAR);
	m_sceneTexture->SetParameter(TextureObject::ParameterEnum::MagFilter, GL_LINEAR);
	Texture2DObject::Unbind();

	// Scene framebuffer
	m_sceneFramebuffer->Bind();
	m_sceneFramebuffer->SetTexture(FramebufferObject::Target::Draw, FramebufferObject::Attachment::Depth, *m_depthTexture);
	m_sceneFramebuffer->SetTexture(FramebufferObject::Target::Draw, FramebufferObject::Attachment::Color0, *m_sceneTexture);
	m_sceneFramebuffer->SetDrawBuffers(std::array<FramebufferObject::Attachment, 1>({ FramebufferObject::Attachment::Color0 }));
	FramebufferObject::Unbind();

	// Add temp textures and frame buffers
	for (int i = 0; i < m_tempFramebuffers.size(); ++i)
	{
		m_tempTextures[i] = std::make_shared<Texture2DObject>();
		m_tempTextures[i]->Bind();
		m_tempTextures[i]->SetImage(0, width, height, TextureObject::FormatRGBA, TextureObject::InternalFormat::InternalFormatRGBA16F);
		m_tempTextures[i]->SetParameter(TextureObject::ParameterEnum::WrapS, GL_CLAMP_TO_EDGE);
		m_tempTextures[i]->SetParameter(TextureObject::ParameterEnum::WrapT, GL_CLAMP_TO_EDGE);
		m_tempTextures[i]->SetParameter(TextureObject::ParameterEnum::MinFilter, GL_LINEAR);
		m_tempTextures[i]->SetParameter(TextureObject::ParameterEnum::MagFilter, GL_LINEAR);

		m_tempFramebuffers[i] = std::make_shared<FramebufferObject>();
		m_tempFramebuffers[i]->Bind();
		m_tempFramebuffers[i]->SetTexture(FramebufferObject::Target::Draw, FramebufferObject::Attachment::Color0, *m_tempTextures[i]);
		m_tempFramebuffers[i]->SetDrawBuffers(std::array<FramebufferObject::Attachment, 1>({ FramebufferObject::Attachment::Color0 }));
	}
	Texture2DObject::Unbind();
	FramebufferObject::Unbind();
}

void ShadowApplication::InitializeRenderer()
{
	int width, height;
	GetMainWindow().GetDimensions(width, height);

	// Add shadow map pass
	if (m_mainLight)
	{
		if (!m_mainLight->GetShadowMap())
		{
			m_mainLight->CreateShadowMap(glm::vec2(2000, 2000), N_OF_CASCADES);
			m_mainLight->SetShadowBias(0.001f); // TODO: dynamic bias
		}
		std::unique_ptr<ShadowMapRenderPass> shadowMapRenderPass(std::make_unique<ShadowMapRenderPass>(m_mainLight, m_shadowMapMaterial));
		glm::vec3 min, max;
		m_scene.GetAABBBounds(min, max);
		shadowMapRenderPass->SetSceneAABBBounds(min, max);
		shadowMapRenderPass->SetCascadeLevels(m_cameraController.GetCamera()->GetCamera()->GetFarPlane());
		m_shadowPassIndex = m_renderer.AddRenderPass(std::move(shadowMapRenderPass));
	}

	// Set up deferred passes
	{
		std::unique_ptr<GBufferRenderPass> gbufferRenderPass(std::make_unique<GBufferRenderPass>(width, height));

		// Set the g-buffer textures as properties of the deferred material
		m_deferredMaterial->SetUniformValue("DepthTexture", gbufferRenderPass->GetDepthTexture());
		m_deferredMaterial->SetUniformValue("AlbedoTexture", gbufferRenderPass->GetAlbedoTexture());
		m_deferredMaterial->SetUniformValue("NormalTexture", gbufferRenderPass->GetNormalTexture());
		m_deferredMaterial->SetUniformValue("OthersTexture", gbufferRenderPass->GetOthersTexture());

		// Get the depth texture from the gbuffer pass - This could be reworked
		m_depthTexture = gbufferRenderPass->GetDepthTexture();

		// Add the render passes
		m_renderer.AddRenderPass(std::move(gbufferRenderPass));
		m_renderer.AddRenderPass(std::make_unique<DeferredRenderPass>(m_deferredMaterial, m_sceneFramebuffer));
	}

	// Initialize the framebuffers and the textures they use
	InitializeFramebuffers();

	// Skybox pass
	m_renderer.AddRenderPass(std::make_unique<SkyboxRenderPass>(m_skyboxTexture));

	// Create a copy pass from m_sceneTexture to the first temporary texture
	std::shared_ptr<Material> copyMaterial = CreatePostFXMaterial("shaders/postfx/copy.frag", m_sceneTexture);
	m_renderer.AddRenderPass(std::make_unique<PostFXRenderPass>(copyMaterial, m_tempFramebuffers[0]));

	// Replace the copy pass with a new bloom pass
	m_bloomMaterial = CreatePostFXMaterial("shaders/postfx/bloom.frag", m_sceneTexture);
	m_bloomMaterial->SetUniformValue("Range", glm::vec2(2.0f, 3.0f));
	m_bloomMaterial->SetUniformValue("Intensity", 1.0f);
	m_renderer.AddRenderPass(std::make_unique<PostFXRenderPass>(m_bloomMaterial, m_tempFramebuffers[0]));

	// Add blur passes
	std::shared_ptr<Material> blurHorizontalMaterial = CreatePostFXMaterial("shaders/postfx/blur.frag", m_tempTextures[0]);
	blurHorizontalMaterial->SetUniformValue("Scale", glm::vec2(1.0f / width, 0.0f));
	std::shared_ptr<Material> blurVerticalMaterial = CreatePostFXMaterial("shaders/postfx/blur.frag", m_tempTextures[1]);
	blurVerticalMaterial->SetUniformValue("Scale", glm::vec2(0.0f, 1.0f / height));
	for (int i = 0; i < m_blurIterations; ++i)
	{
		m_renderer.AddRenderPass(std::make_unique<PostFXRenderPass>(blurHorizontalMaterial, m_tempFramebuffers[1]));
		m_renderer.AddRenderPass(std::make_unique<PostFXRenderPass>(blurVerticalMaterial, m_tempFramebuffers[0]));
	}

	// Final pass
	m_composeMaterial = CreatePostFXMaterial("shaders/postfx/compose.frag", m_sceneTexture);

	// Set exposure uniform default value
	m_composeMaterial->SetUniformValue("Exposure", m_exposure);

	// Set uniform default values
	m_composeMaterial->SetUniformValue("Contrast", m_contrast);
	m_composeMaterial->SetUniformValue("HueShift", m_hueShift);
	m_composeMaterial->SetUniformValue("Saturation", m_saturation);
	m_composeMaterial->SetUniformValue("ColorFilter", m_colorFilter);

	// Set the bloom texture uniform
	m_composeMaterial->SetUniformValue("BloomTexture", m_tempTextures[0]);

	m_renderer.AddRenderPass(std::make_unique<PostFXRenderPass>(m_composeMaterial, m_renderer.GetDefaultFramebuffer()));
}

std::shared_ptr<Material> ShadowApplication::CreatePostFXMaterial(const char* fragmentShaderPath, std::shared_ptr<Texture2DObject> sourceTexture)
{
	// We could keep this vertex shader and reuse it, but it looks simpler this way
	std::vector<const char*> vertexShaderPaths;
	vertexShaderPaths.push_back("shaders/version330.glsl");
	vertexShaderPaths.push_back("shaders/renderer/fullscreen.vert");
	Shader vertexShader = ShaderLoader(Shader::VertexShader).Load(vertexShaderPaths);

	std::vector<const char*> fragmentShaderPaths;
	fragmentShaderPaths.push_back("shaders/version330.glsl");
	fragmentShaderPaths.push_back("shaders/utils.glsl");
	fragmentShaderPaths.push_back(fragmentShaderPath);
	Shader fragmentShader = ShaderLoader(Shader::FragmentShader).Load(fragmentShaderPaths);

	std::shared_ptr<ShaderProgram> shaderProgramPtr = std::make_shared<ShaderProgram>();
	shaderProgramPtr->Build(vertexShader, fragmentShader);

	// Create material
	std::shared_ptr<Material> material = std::make_shared<Material>(shaderProgramPtr);
	material->SetUniformValue("SourceTexture", sourceTexture);

	return material;
}

Renderer::UpdateTransformsFunction ShadowApplication::GetFullscreenTransformFunction(std::shared_ptr<ShaderProgram> shaderProgramPtr) const
{
	// Get transform related uniform locations
	ShaderProgram::Location invViewMatrixLocation = shaderProgramPtr->GetUniformLocation("InvViewMatrix");
	ShaderProgram::Location invProjMatrixLocation = shaderProgramPtr->GetUniformLocation("InvProjMatrix");
	ShaderProgram::Location worldViewProjMatrixLocation = shaderProgramPtr->GetUniformLocation("WorldViewProjMatrix");

	// Return transform function
	return [=](const ShaderProgram& shaderProgram, const glm::mat4& worldMatrix, const Camera& camera, bool cameraChanged)
		{
			if (cameraChanged)
			{
				shaderProgram.SetUniform(invViewMatrixLocation, glm::inverse(camera.GetViewMatrix()));
				shaderProgram.SetUniform(invProjMatrixLocation, glm::inverse(camera.GetProjectionMatrix()));
			}
			shaderProgram.SetUniform(worldViewProjMatrixLocation, camera.GetViewProjectionMatrix() * worldMatrix);
		};
}

void ShadowApplication::RenderGUI()
{
	m_imGui.BeginFrame();

	// Draw GUI for scene nodes, using the visitor pattern
	ImGuiSceneVisitor imGuiVisitor(m_imGui, "Scene");
	m_scene.AcceptVisitor(imGuiVisitor);

	// Draw GUI for camera controller
	m_cameraController.DrawGUI(m_imGui);

	if (auto window = m_imGui.UseWindow("Post FX"))
	{
		if (m_composeMaterial)
		{
			if (ImGui::DragFloat("Exposure", &m_exposure, 0.01f, 0.01f, 5.0f))
			{
				m_composeMaterial->SetUniformValue("Exposure", m_exposure);
			}

			ImGui::Separator();

			if (ImGui::SliderFloat("Contrast", &m_contrast, 0.5f, 1.5f))
			{
				m_composeMaterial->SetUniformValue("Contrast", m_contrast);
			}
			if (ImGui::SliderFloat("Hue Shift", &m_hueShift, -0.5f, 0.5f))
			{
				m_composeMaterial->SetUniformValue("HueShift", m_hueShift);
			}
			if (ImGui::SliderFloat("Saturation", &m_saturation, 0.0f, 2.0f))
			{
				m_composeMaterial->SetUniformValue("Saturation", m_saturation);
			}
			if (ImGui::ColorEdit3("Color Filter", &m_colorFilter[0]))
			{
				m_composeMaterial->SetUniformValue("ColorFilter", m_colorFilter);
			}

			ImGui::Separator();

			if (ImGui::DragFloat2("Bloom Range", &m_bloomRange[0], 0.1f, 0.1f, 10.0f))
			{
				m_bloomMaterial->SetUniformValue("Range", m_bloomRange);
			}
			if (ImGui::DragFloat("Bloom Intensity", &m_bloomIntensity, 0.1f, 0.0f, 5.0f))
			{
				m_bloomMaterial->SetUniformValue("Intensity", m_bloomIntensity);
			}
		}
	}

	// Debug window for the light camera
	//if (auto window = m_imGui.UseWindow("Shadow Debug"))
	//{
	//	ImVec2 pos = ImGui::GetCursorScreenPos();
	//	ImVec2 wsize = ImGui::GetWindowSize();
	//	glm::vec2 res = m_mainLight->GetShadowMapResolution();

	//	ImVec2 minCorner = ImVec2(pos.x, pos.y);
	//	float fitX = wsize.x / res.x;
	//	float fitY = wsize.y / res.y;
	//	float fit = std::min(fitX, fitY);
	//	ImVec2 maxCorner = ImVec2(fit * res.x + pos.x, fit * res.y + pos.y);

	//	ImGui::GetWindowDrawList()->AddImage(
	//		(ImTextureID)m_mainLight->GetShadowMap()->GetHandle(), // Image handle
	//		minCorner, maxCorner // Image size
	//		, ImVec2(0, 1), ImVec2(1, 0)); // UV coords (flipped)
	//}

	m_imGui.EndFrame();
}

void ShadowApplication::CreateTerrainMaterial(std::shared_ptr<Material> material)
{
	material->SetUniformValue<glm::vec3>("Color", m_terrainColor);

	m_terrain_colorTexture = LoadTexture("textures/grass.jpg");
	material->SetUniformValue("ColorTexture", m_terrain_colorTexture);

	m_terrain_normalTexture = LoadTexture("textures/Default_NormalMap.png");
	material->SetUniformValue("NormalTexture", m_terrain_normalTexture);

	m_terrain_specularTexture = LoadTexture("textures/Default_ARM.png");
	material->SetUniformValue("SpecularTexture", m_terrain_specularTexture);
}

std::shared_ptr<Texture2DObject> ShadowApplication::LoadTexture(const char* path)
{
	std::shared_ptr<Texture2DObject> texture = std::make_shared<Texture2DObject>();

	int width = 0;
	int height = 0;
	int components = 0;

	// Load the texture data here
	unsigned char* data = stbi_load(path, &width, &height, &components, 4);

	texture->Bind();
	texture->SetImage(0, width, height, TextureObject::FormatRGBA, TextureObject::InternalFormatRGBA, std::span<const unsigned char>(data, width * height * 4));

	// Generate mipmaps
	texture->GenerateMipmap();

	// Release texture data
	stbi_image_free(data);

	return texture;
}