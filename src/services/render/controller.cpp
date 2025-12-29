#include "controller.hpp"

#include "pages/[pages_loader].hpp"

namespace Core::App::Render
{
    void Controller::Initialise()
    {
        window_.setVerticalSyncEnabled(true);
        window_.setView(view_);
    }

    void Controller::OnAllInterfacesLoaded()
    {
        game_ = IFace().Get<Game>();
    }

    void Controller::ProcessTick()
    {
        UpdateScene();
    }

    void Controller::UpdateScene()
    {
        if (window_.isOpen())
        {
            sf::Event event {};
            while (window_.pollEvent(event))
            {
                switch (event.type)
                {
                    case sf::Event::Closed:
                        window_.close();
                        Log()->Debug("Close window_");

                        std::exit(0);
                        break;
                    default:
                        break;
                }

                for (const auto& baseService: Pages::PagesLoader().Services())
                {
                    const auto page = std::dynamic_pointer_cast<Pages::PagesServiceInstance>(baseService);
                    if (page && page->GetType() == game_->GetMainState())
                    {
                        page->HandleEvent(event, window_);
                    }
                }
            }

            // Отрисовка
            window_.clear(sf::Color(27, 24, 82)); // Фон

            for (const auto& baseService: Pages::PagesLoader().Services())
            {
                const auto page = std::dynamic_pointer_cast<Pages::PagesServiceInstance>(baseService);
                if (page && page->GetType() == game_->GetMainState())
                {
                    page->UpdateScene();
                }
            }

            window_.display();
        }


        // drawable.clear();
    }

    sf::RenderWindow & Controller::Window()
    {
        return window_;
    }

    std::vector<Utils::Service::SubLoader> Controller::SubLoaders()
    {
        static Pages::PagesServiceContainer container{shared_from_this()};

        return
        {
            {
                Pages::PagesLoader(),
                [](auto child_) {
                    const auto child = dynamic_cast<Pages::PagesServiceContainer*>(child_);
                    child->SetupContainer(container);
                }
            }
        };
    };
}