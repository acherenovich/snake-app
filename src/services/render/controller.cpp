#include "controller.hpp"
#include "pages/[pages_loader].hpp"

namespace Core::App::Render
{
    void Controller::Initialise()
    {
        window_.create(
            sf::VideoMode(
                sf::Vector2u(
                    static_cast<unsigned int>(Width),
                    static_cast<unsigned int>(Height)
                )
            ),
            "Snake",
            sf::Style::Titlebar | sf::Style::Close,
            sf::State::Windowed,
            settings_
        );

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
        if (!window_.isOpen())
        {
            return;
        }

        while (auto event = window_.pollEvent())
        {
            // ✅ SFML3: проверка типа через is<>
            if (event->is<sf::Event::Closed>())
            {
                window_.close();
                Log()->Debug("Close window_");

                std::exit(0);
            }

            for (const auto& baseService: Pages::PagesLoader().Services())
            {
                const auto page = std::dynamic_pointer_cast<Pages::PagesServiceInstance>(baseService);
                if (page && page->GetType() == game_->GetMainState())
                {
                    page->HandleEvent(*event, window_); // ✅ разыменовываем optional
                }
            }
        }

        window_.clear(sf::Color(27, 24, 82));

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

    sf::RenderWindow & Controller::Window()
    {
        return window_;
    }

    std::vector<Utils::Service::SubLoader> Controller::SubLoaders()
    {
        static Pages::PagesServiceContainer container {shared_from_this()};

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
    }
}
