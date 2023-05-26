#include <LibLOG.h>
#include "WindowManager.h"
#include "FormManager.h"
#include "FrontEndMain.h"

bool ProcessKeyDownShortcuts(Rml::Context* context, Rml::Input::KeyIdentifier key, int key_modifier, float native_dp_ratio, bool priority)
{
	if (!context) return true;

	// Result should return true to allow the event to propagate to the next handler.
	bool result = false;

	// This function is intended to be called twice by the backend, before and after submitting the key event to the context. This way we can
	// intercept shortcuts that should take priority over the context, and then handle any shortcuts of lower priority if the context did not
	// intercept it.
	if (priority)
	{
		// Priority shortcuts are handled before submitting the key to the context.

		// Toggle debugger and set dp-ratio using Ctrl +/-/0 keys.
		if (key == Rml::Input::KI_F8)
		{
			Rml::Debugger::SetVisible(!Rml::Debugger::IsVisible());
		}
		else if (key == Rml::Input::KI_0 && key_modifier & Rml::Input::KM_CTRL)
		{
			context->SetDensityIndependentPixelRatio(native_dp_ratio);
		}
		else if (key == Rml::Input::KI_1 && key_modifier & Rml::Input::KM_CTRL)
		{
			context->SetDensityIndependentPixelRatio(1.f);
		}
		else if ((key == Rml::Input::KI_OEM_MINUS || key == Rml::Input::KI_SUBTRACT) && key_modifier & Rml::Input::KM_CTRL)
		{
			const float new_dp_ratio = Rml::Math::Max(context->GetDensityIndependentPixelRatio() / 1.2f, 0.5f);
			context->SetDensityIndependentPixelRatio(new_dp_ratio);
		}
		else if ((key == Rml::Input::KI_OEM_PLUS || key == Rml::Input::KI_ADD) && key_modifier & Rml::Input::KM_CTRL)
		{
			const float new_dp_ratio = Rml::Math::Min(context->GetDensityIndependentPixelRatio() * 1.2f, 2.5f);
			context->SetDensityIndependentPixelRatio(new_dp_ratio);
		}
		else
		{
			// Propagate the key down event to the context.
			result = true;
		}
	}
	else
	{
		// We arrive here when no priority keys are detected and the key was not consumed by the context. Check for shortcuts of lower priority.
		if (key == Rml::Input::KI_R && key_modifier & Rml::Input::KM_CTRL)
		{
			for (int i = 0; i < context->GetNumDocuments(); i++)
			{
				Rml::ElementDocument* document = context->GetDocument(i);
				const Rml::String& src = document->GetSourceURL();
				if (src.size() > 4 && src.substr(src.size() - 4) == ".rml")
				{
					document->ReloadStyleSheet();
				}
			}
		}
		else
		{
			result = true;
		}
	}

	return result;
}

int FrontendMain()
{
    int window_width = SETTINGS->GetProperty<int>("Resolution","GameResolutionX", 1024);
	int window_height = SETTINGS->GetProperty<int>("Resolution","GameResolutionY", 768);
    SDL_SetMainReady();
    SDL_Init(SDL_INIT_VIDEO);

    //Инициализируем наш rmlui бекенд
    bool isbackendinitialized  = Backend::Initialize("OpenSRDemo", window_width, window_height, 1, SETTINGS->GetProperty<bool>("Resolution","Fullscreen", false) );
    Rml::SetSystemInterface(Backend::GetSystemInterface());
	Rml::SetRenderInterface(Backend::GetRenderInterface());

    Rml::Initialise();
	

	//Создаём контекстный менеджер, и создаём на нём один контекст
	

    Rml::Context* context = Rml::CreateContext("main", Rml::Vector2i(window_width, window_height));
    if (!context)
	{
		Rml::Shutdown();
		Backend::Shutdown();
		return -1;
	}

	//TODO Сделать Fonts manager
	//Загрузка основного шрифта
    bool isfontloaded = Rml::LoadFontFace(fs::path(FONTSPATH / fs::path("boucle.otf")).string());



    Rml::Debugger::Initialise(context);
	
	WindowManager* windowManager = new WindowManager(context);
	Rml::ElementDocument* mainwindow = windowManager->Add(UIPATH / fs::path("MainMenu.rml"));
	FormManager* formManager = new FormManager(context);
	Rml::ElementDocument* mainform = formManager->Add(UIPATH / fs::path("Settings.rml"));
	Rml::ElementDocument* mainform2 = formManager->Add(UIPATH / fs::path("Test.rml"));
	Rml::ElementDocument* mainform3 = formManager->Add(UIPATH / fs::path("Test2.rml"));

	// получаем все элементы img в документе
	Rml::ElementList imgElements;
	mainwindow->GetElementsByTagName(imgElements, "img");
	for (auto& element : imgElements)
	{
		if (element->HasAttribute("src"))
		{
			string srcAttribute = element->GetAttribute("src")->Get<string>();
			boost::replace_all(srcAttribute, "{RESPATH}", RESPATH.string());
			boost::replace_all(srcAttribute, "{DATAPATH}", DATAPATH.string());
			element->SetAttribute("src", srcAttribute);
		}
	}
	mainwindow->Show();
	mainform->Show();
	mainform2->Show();
	mainform3->Show();

    bool running = true;
    while (running)
    {
		//Загружаем счётчик FPS, если в настройках включено его отображение
        running = Backend::ProcessEvents(context, ProcessKeyDownShortcuts, true);
        context->Update();

		Backend::BeginFrame();
		context->Render();
		Backend::PresentFrame();
    }

    context->UnloadAllDocuments();
    Rml::Shutdown();
    Backend::Shutdown();
    return 0;
}