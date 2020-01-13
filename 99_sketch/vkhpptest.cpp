#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.hpp"

HWND win_init(const char *appname, int Width, int Height) {
	auto WindowProc = [](HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT {
		int temp = wParam & 0xFFF0;
		switch (msg) {
		case WM_SYSCOMMAND:
			if (temp == SC_MONITORPOWER || temp == SC_SCREENSAVE) {
				return 0;
			}
			break;
		case WM_IME_SETCONTEXT:
			lParam &= ~ISC_SHOWUIALL;
			return 0;
			break;
		case WM_CLOSE:
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
			break;
		case WM_KEYDOWN:
			if (wParam == VK_ESCAPE) PostQuitMessage(0);
			break;
		case WM_SIZE:
			break;
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);
	};
	static WNDCLASSEX wcex = {
		sizeof(WNDCLASSEX), CS_CLASSDC | CS_VREDRAW | CS_HREDRAW,
		WindowProc, 0L, 0L, GetModuleHandle(NULL), LoadIcon(NULL, IDI_APPLICATION),
		LoadCursor(NULL, IDC_ARROW), (HBRUSH)GetStockObject(BLACK_BRUSH),
		NULL, appname, NULL
	};
	wcex.hInstance = GetModuleHandle(NULL);
	RegisterClassEx(&wcex);

	DWORD styleex = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	DWORD style = WS_OVERLAPPEDWINDOW;
	style &= ~(WS_SIZEBOX | WS_MAXIMIZEBOX | WS_MINIMIZEBOX);

	RECT rc;
	SetRect( &rc, 0, 0, Width, Height );
	AdjustWindowRectEx( &rc, style, FALSE, styleex);
	rc.right -= rc.left;
	rc.bottom -= rc.top;
	HWND  hWnd = CreateWindowEx(styleex, appname, appname, style, 0, 0, rc.right, rc.bottom, NULL, NULL, wcex.hInstance, NULL);
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);
	ShowCursor(TRUE);
	return hWnd;
}

BOOL win_proc_msg() {
	MSG msg;
	BOOL IsActive = TRUE;
	while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) IsActive = FALSE;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return IsActive;
}

class Resource {
public:
	Resource() {
	}
	virtual ~Resource() {
	}
};

class Vertex : public Resource {
	int vertex_count = 0;
public:
	Vertex(ResourceManager *manager, void *data, size_t size, size_t stride_size) {

	}
	int GetCount() {
		return vertex_count;
	}
};


//
//あらゆるメモリやBindするメモリ、descなどのsetsはここで管理することにする
//
class ResourceManager {
public:
	ResourceManager(vk::DeviceSize image_size, vk::DeviceSize desc_pool_size) {
	}
	
	~ResourceManager() {
	}
};

class Shader : public Resource {
public:
	~Shader() {
	}

	
	// https://github.com/google/skia/blob/master/src/gpu/vk/GrVkPipelineState.cpp
	//
	//めんどくさいので全部こいつに閉じ込めたい気がする
	// シェーダーには何を入力/出力するのかが書いてあるけど、それはあくまでもGLSLとかそっちレベルの話
	// GPU側には、その識別子自体の入出力をするためのリソース確保(descripterSetsはPoolから作られる)
	// レイアウトとかそういうのってシェーダー側と結合度がめちゃんこ高いので、
	// Shader内部のクラスとして管理した方が絶対に良いと思ってる。
	// deviceに必要なものは、descripterPool
	//
	//
	Shader(ResourceManager *manager, const char *vsfile, const char *fsfile, uint32_t state) { 
	}

	GLuint Get() {
		return prog;
	}
};

class Texture : public Resource {
public:
	~Texture() {
	}
	Texture(ResourceManager *manager, int w, int h, int format = TextureBuffer::FORMAT_RGBA8, const void *data = nullptr) {
};

class RenderTarget : public Resource {
	Texture *textures[MAX] = {};
	int Width = 0;
	int Height = 0;
	//AND framebuffer
public:
	Texture *GetTexture(int index = 0) {
		return textures[index];
	}

	~RenderTarget() {
	}

	RenderTarget(ResourceManager *manager, int w, int h, int count) {
	}
};

class Renderer {
	vk::ApplicationInfo appInfo;
	vk::InstanceCreateInfo instanceInfo;
	vk::Instance instance;
	std::vector<vk::PhysicalDevice> physicalDevices;
	std::vector<vk::QueueFamilyProperties> queueFamilyProperties;
	vk::DeviceQueueCreateInfo queueCreateInfo;
	vk::Device device;
	vk::Queue  queue ;
	vk::SurfaceKHR surface;
	std::vector<vk::SurfaceFormatKHR> surfaceFormats;
	std::vector<vk::PresentModeKHR> presentModes;
	vk::SurfaceCapabilitiesKHR surfCaps;
	vk::SwapchainKHR swapChain ;
	std::vector<vk::Image> swapImages;
	
	std::map<std::string, BaseBuffer *> buffermapper;
	template <typename T>
	T *GetBuffer(const std::string &name) {
		T *ret = (T *)FindBuffer(name);
		if (!ret) {
			ret = new T;
			ret->SetName(name);
			buffermapper[name] = ret;
		}
		return ret;
	}

public:

	ViewBuffer *GetViewBuffer(const std::string &name) {
		return GetBuffer<ViewBuffer>(name);
	}

	VertexBuffer *GetVertexBuffer(const std::string &name) {
		return GetBuffer<VertexBuffer>(name);
	}

	TextureBuffer *GetTextureBuffer(const std::string &name) {
		return GetBuffer<TextureBuffer>(name);
	}

	RenderTargetBuffer *GetRenderTargetBuffer(const std::string &name) {
		return GetBuffer<RenderTargetBuffer>(name);
	}

	void MarkUpdate(int type) {
		auto it = buffermapper.begin();
		auto ite = buffermapper.end();
		for (; it != ite; it++) {
			auto *buf = it->second;
			if (buf->GetType() == type) {
				buf->MarkUpdate();
			}
		}
	}
	//Update
	//頂点バッファとかそういうの
	void Update() {
		//レンダリングしようと試みているデータを更新する
		//Buffer側では、レンダリングするデータをシステムメモリに置いてあるけど
		//VKでは、
		//Bufferハンドルを作成 -> バインドするメモリ領域を作成、ないし選択 -> 選択してBind -> 必要に応じてMapしてデータ転送
		//システム側からローカル側へデータを転送するので、システム側とGPU側でデータが見える領域を作成して、TRANSFERを行う
		
		//レンダリングしようとして先を取ってきて存在しないなら、
		//Images + Depthでrenderpass + framebufferを作成。そんだけ。シンプル
		//
	}
	
	//Render
	//画面にでるフレームバッファとかそういうのの実instance作成と更新
	void Render(std::string view_name) {
		ViewBuffer *view = (ViewBuffer *)buffermapper[view_name];
		if(!view) return;
		int rtvCount = 1;
		auto *rt = view->GetRenderTarget();
		RenderTarget *curr_rt = NULL;
		if (rt) {
			rtvCount = rt->GetCount();
			std::string rt_name = rt->GetName();
			if (cache_rendertarget.find(rt_name) == cache_rendertarget.end()) {
				auto *rendertarget = new RenderTarget(nullptr, rt->GetWidth(), rt->GetHeight(), rtvCount);
				for (int i = 0 ; i < rtvCount; i++) {
					Texture *texture = rendertarget->GetTexture(i);
					TextureBuffer *color_tex = rt->GetTexture(i, false);
					cache_texture[color_tex->GetName()] = texture;
				}
				cache_rendertarget[rt_name] = rendertarget;
			}
			curr_rt = cache_rendertarget[rt_name];
		}

		//レンダーターゲット指定されているならフレームバッファ設定
		if(curr_rt) {
			glBindFramebuffer(GL_FRAMEBUFFER, curr_rt->Get());
			std::vector<GLenum> DrawBuffers;
			for(int i = 0 ; i < curr_rt->GetCount(); i++) {
				DrawBuffers.push_back(GL_COLOR_ATTACHMENT0 + i);
			}
			glDrawBuffers(curr_rt->GetCount(), &DrawBuffers[0]);
			int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if(status != GL_FRAMEBUFFER_COMPLETE ) {
				printf("ERROR GL_FRAMEBUFFER_COMPLETE Failed : status=%08X\n", status);
			}
		}

		//Viewports
		float clear_color[4] = {};
		view->GetClearColor(clear_color);
		glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, view->GetWidth(), view->GetHeight());
		glScissor(0, 0, view->GetWidth(), view->GetHeight());

		glEnable(GL_TEXTURE_2D);
		glEnable(GL_DEPTH_TEST);

		//per viewports
		bool is_multi_rendering = false;
		size_t viewport_count = view->GetViewportSize();
		if(viewport_count > 0) {
			is_multi_rendering = true;
		} else {
			viewport_count = 1;
		}
		std::vector<VertexBuffer *> temp_vertex_buffer;
		view->GetVertexBuffer(temp_vertex_buffer);
		for(size_t i = 0 ; i < viewport_count; i++) {
			
			//ビューポート、シザーを取得して設定
			ViewBuffer::ViewportData viewport_data;
			MatrixData matrix_proj  ;
			MatrixData matrix_view  ;
			MatrixData matrix_world ;
			if(is_multi_rendering) {
				view->GetViewportData(i, &viewport_data);
				matrix_proj  = viewport_data.proj;
				matrix_view  = viewport_data.view;
				matrix_world = viewport_data.world;
			} else {
				viewport_data.x = 0.0;
				viewport_data.y = 0.0;
				viewport_data.w = view->GetWidth();
				viewport_data.h = view->GetHeight();
				matrix_proj  = view->GetMatrix("proj");
				matrix_view  = view->GetMatrix("view");
				matrix_world = view->GetMatrix("world");
				
				
			}
			glViewport(viewport_data.x, viewport_data.y, viewport_data.w, viewport_data.h);
			glScissor(viewport_data.x, viewport_data.y, viewport_data.w, viewport_data.h);
			
			for (auto vertex : temp_vertex_buffer) {
				auto buf = vertex;
				
				//トポロジーを保存しておく
				auto topo = buf->GetTopologie();

				//親のその他諸々を取得しておく
				std::map<std::string, MatrixData> buf_matrix;
				std::map<std::string, VectorData> buf_vectors;
				//親のその他諸々を取得しておく
				buf->GetMatrix(buf_matrix);
				buf->GetVector(buf_vectors);
				std::string shader_name = buf->GetShaderName();

				//シェーダー作成
				if (!shader_name.empty()) {
					auto it = cache_shader.find(shader_name);
					if (it == cache_shader.end()) {
						Shader *shader = new Shader(
							nullptr,
							(shader_name + std::string(".vsh")).c_str(),
							(shader_name + std::string(".fsh")).c_str());
						auto it = cache_shader.find(shader_name);
						if (it != cache_shader.end()) {
							delete it->second;
						}
						cache_shader[shader_name] = shader;
					}
				}

				//シェーダー設定
				auto shader_object = cache_shader[shader_name];
				if (!shader_object) {
					printf("シェーダーが設定されていない : %s\n", shader_name.c_str());
					
					buf = (VertexBuffer *)buf->GetChild();
					continue;
				}

				//シェーダー設定
				auto prog = shader_object->Get();
				glUseProgram(prog);

				//ビュー側のuniform設定
				{
					std::map<std::string, MatrixData> matrix;
					view->GetMatrix(matrix);
					for(auto &m : matrix) {
						auto name = m.first.c_str();
						auto data = m.second;
						GLuint loc = glGetUniformLocation(prog, name);
						glUniformMatrix4fv(loc, 1, GL_FALSE, (const GLfloat *)&data);
					}
				}
				{
					std::map<std::string, VectorData> vectors;
					view->GetVector(vectors);
					for(auto &m : vectors) {
						auto name = m.first.c_str();
						auto data = m.second;
						GLuint loc = glGetUniformLocation(prog, name);
						glUniform4fv(loc, 1, (const GLfloat *)&data);
					}
				}
				//特別扱い
				{
					GLuint loc = glGetUniformLocation(prog, "proj");
					glUniformMatrix4fv(loc, 1, GL_FALSE, (const GLfloat *)&matrix_proj);
				}
				{
					GLuint loc = glGetUniformLocation(prog, "view");
					glUniformMatrix4fv(loc, 1, GL_FALSE, (const GLfloat *)&matrix_view);
				}
				{
					GLuint loc = glGetUniformLocation(prog, "world");
					glUniformMatrix4fv(loc, 1, GL_FALSE, (const GLfloat *)&matrix_world);
				}

				//VertexBuffer側のuniform設定
				for(auto &m : buf_matrix) {
					auto name = m.first.c_str();
					auto data = m.second;
					GLuint loc = glGetUniformLocation(prog, name);
					glUniformMatrix4fv(loc, 1, GL_FALSE, (const GLfloat *)&data);
				}
				for(auto &m : buf_vectors) {
					auto name = m.first.c_str();
					auto data = m.second;
					GLuint loc = glGetUniformLocation(prog, name);
					glUniform4fv(loc, 1, (const GLfloat *)&data);
				}
				
				int index = 0;
				while(buf) {
					//頂点バッファ設定
					auto vertex_object = cache_vertex[buf->GetName()];
					if (!vertex_object) {
						auto str = vertex->GetName();
						printf("頂点バッファがありません : %s\n", str.c_str());

						buf = (VertexBuffer *)buf->GetChild();
						continue;
					}
					glBindVertexArray(vertex_object->Get());

					//テクスチャ設定
					std::map<std::string, std::string> uniform_texture_name;
					buf->GetTextureFile(uniform_texture_name);
					int index = 0;
					for(auto &tex : uniform_texture_name) {
						auto name = tex.first.c_str();
						auto filename = tex.second.c_str();

						//printf("name=%s, filename=%s\n", name, filename);

						auto texture_object = cache_texture[filename];
						if(texture_object) {
							GLuint loc = glGetUniformLocation(prog, name);
							glActiveTexture(GL_TEXTURE0 + index);
							glBindTexture(GL_TEXTURE_2D, texture_object->Get());
							glUniform1i(loc, index);
						} else {
							auto nulltexture = cache_texture["nulltexture"];
							if(nulltexture) {
								GLuint loc = glGetUniformLocation(prog, name);
								glActiveTexture(GL_TEXTURE0 + index);
								glBindTexture(GL_TEXTURE_2D, nulltexture->Get());
								glUniform1i(loc, index);
								//printf("loc=%d\, index=%d, texhandle = %llX\n", loc, index, nulltexture->Get());
							}
						}
						index++;
					}

					//printf("topo = %d\n", topo);
					if(topo == VertexBuffer::TRIANGLES) {
						glDrawArrays(GL_TRIANGLES, 0, vertex_object->GetCount());
					}
					
					if(topo == VertexBuffer::LINES) {
						glDrawArrays(GL_LINES, 0, vertex_object->GetCount());
					}
					if(topo == VertexBuffer::LINE_STRIP) {
						glDrawArrays(GL_LINE_STRIP, 0, vertex_object->GetCount());
					}

					if(topo == VertexBuffer::POINTS) {
						glDrawArrays(GL_POINTS, 0, vertex_object->GetCount());
					}

					//子も表示
					buf = (VertexBuffer *)buf->GetChild();
				}
				//printf("buf index=%d, buf=%llX\n", index++, buf);
				glUseProgram(0);
				
			}

			glBindVertexArray(0);
		}
		//フレームバッファバインド終わり
		if(curr_rt) {
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	}
	
	Renderer(const char *appName, const char *engineName, HWND hWnd, HINSTANCE hInstance, uint32_t width, uint32_t height) {

		///vulkan
		std::vector<const char *> layerNames = {
			"VK_LAYER_LUNARG_standard_validation",
		};
		std::vector<const char *> extSwap = { "VK_KHR_swapchain" };
		std::vector<const char *> extNames = {
			VK_KHR_SURFACE_EXTENSION_NAME,
			VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
			VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
		};

		//instance作成
		appInfo = vk::ApplicationInfo(appName, 1, engineName, 1, VK_MAKE_VERSION(1,0,30));
		instanceInfo = vk::InstanceCreateInfo({}, &appInfo, layerNames.size(), layerNames.data(), extNames.size(), extNames.data());
		vk::createInstance(&instanceInfo, nullptr, &instance);

		//物理デバイス作成
		physicalDevices = instance.enumeratePhysicalDevices();
		if(physicalDevices.empty()) {
			printf("ぐらぼがしんでる\n");
		}
		vk::PhysicalDevice physicalDevice = physicalDevices[0];
		queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
		uint32_t graphicsQueueFamilyIndex = 0;
		for (size_t i = 0; i < queueFamilyProperties.size(); i++) {
			if (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) {
				graphicsQueueFamilyIndex = i;
				break;
			}
		}

		//create device and queue
		const float defaultQueuePriority(0.0f);
		queueCreateInfo = vk::DeviceQueueCreateInfo(vk::DeviceQueueCreateFlags(), graphicsQueueFamilyIndex, 1, &defaultQueuePriority);
		std::vector<const char *> deviceExtensions;
		deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		vk::DeviceCreateInfo deviceCreateInfo;
		deviceCreateInfo.queueCreateInfoCount = 1;
		deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
		deviceCreateInfo.pEnabledFeatures = nullptr;
		if (deviceExtensions.size() > 0) {
			deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
			deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
		}
		device = physicalDevice.createDevice(deviceCreateInfo);
		queue = device.getQueue(graphicsQueueFamilyIndex, 0);

		
		//Create Swapchain
		vk::Win32SurfaceCreateInfoKHR surfaceCreateInfo;
		surfaceCreateInfo.hinstance = (HINSTANCE)hInstance;
		surfaceCreateInfo.hwnd = (HWND)hWnd;
		surface = instance.createWin32SurfaceKHR(surfaceCreateInfo);
		surfaceFormats = physicalDevice.getSurfaceFormatsKHR(surface);
		surfCaps = physicalDevice.getSurfaceCapabilitiesKHR(surface);
		presentModes = physicalDevice.getSurfacePresentModesKHR(surface);
		std::vector<vk::Bool32> supportsPresent(queueFamilyProperties.size());
		for(size_t i = 0; i < queueFamilyProperties.size(); i++)
		{
			supportsPresent[i] = physicalDevice.getSurfaceSupportKHR(i, surface);
		}
		vk::SwapchainCreateInfoKHR swapchainCI;
		swapchainCI.surface = surface;
		swapchainCI.minImageCount = surfCaps.minImageCount;
		swapchainCI.imageFormat = vk::Format::eB8G8R8A8Unorm;;
		swapchainCI.imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
		swapchainCI.imageExtent.width = width;
		swapchainCI.imageExtent.height = height;
		swapchainCI.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
		swapchainCI.imageArrayLayers = 1;
		swapchainCI.imageSharingMode = vk::SharingMode::eExclusive;
		swapchainCI.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
		swapchainCI.queueFamilyIndexCount = 0;
		swapchainCI.pQueueFamilyIndices = NULL;
		swapchainCI.presentMode = vk::PresentModeKHR::eFifo;
		swapchainCI.clipped = VK_TRUE;
		swapchainCI.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
		swapchainCI.oldSwapchain = vk::SwapchainKHR();

		swapChain = device.createSwapchainKHR(swapchainCI);
		
		//Image取得
		swapImages = device.getSwapchainImagesKHR(swapChain);

		printf("device=%llX\n", device);
		printf("queue=%llX\n", queue);
		printf("surface=%llX\n", queue);
		printf("surfaceFormats.size()=%llX\n", surfaceFormats.size());
		for(auto &img : swapImages) {
			printf("images=%llX\n", img);
		}

	}
};

int main() {
	const char *appName = "vkhpptest.cpp";
	const char *engineName = "vkhpptest_engine.cpp";
	const int width = 1280;
	const int height = 720;
	auto hWnd = win_init(appName, width, height);
	auto hInstance = GetModuleHandle(nullptr);
	auto renderer = new Renderer(appName, engineName, hWnd, hInstance, width, height);

	return 0;
}
