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
                // float cameraSpeed = 10.f; // Скорость движения камеры (пиксели/секунду)

                // Обработка событий
                switch (event.type)
                {
                    case sf::Event::Closed:
                        window_.close();
                        Log()->Debug("Close window_");
                        // initializer.globalEvents.CallEvent(CallStop);
                        break;

                    case sf::Event::KeyPressed:
                        if (event.key.code == sf::Keyboard::Escape)
                        {
                            window_.close();
                            Log()->Debug("Close window_");
                            // initializer.globalEvents.CallEvent(CallStop);
                        }
                        else if (event.key.code == sf::Keyboard::Space)
                        {
                            // Добавьте логику для обработки Space
                        }
                        break;
                    case sf::Event::TextEntered:
                        // if (game->State() == GameMenu && isInputFocused)
                        // {
                        //     if (event.text.unicode < 128)
                        //     {
                        //         if (event.text.unicode == 8) // Backspace
                        //         {
                        //             if (!nicknameString.empty())
                        //                 nicknameString.pop_back();
                        //         }
                        //         else if (event.text.unicode >= 32 && event.text.unicode <= 126) // Печатаемые символы
                        //         {
                        //             if (nicknameString.length() < 20)
                        //                 nicknameString += static_cast<char>(event.text.unicode);
                        //         }
                        //         else if (event.text.unicode == 13) // Enter
                        //         {
                        //             game->StartGame(nicknameString);
                        //         }
                        //     }
                        // }
                        break;
                    case sf::Event::MouseButtonPressed:
                        // if (game->State() == GameMenu && event.mouseButton.button == sf::Mouse::Left)
                        // {
                        //     sf::Vector2f mousePos = window_.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
                        //
                        //     if (inputField.getGlobalBounds().contains(mousePos))
                        //     {
                        //         isInputFocused = true;
                        //     }
                        //     else if (playButton.getGlobalBounds().contains(mousePos))
                        //     {
                        //         game->StartGame(nicknameString);
                        //     }
                        //     else
                        //     {
                        //         isInputFocused = false;
                        //     }
                        // }
                        break;

                    default:
                        break;
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