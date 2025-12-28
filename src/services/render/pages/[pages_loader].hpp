#pragma once

#include <utility>

#include "services/game/interfaces/controller.hpp"
#include "services/render/controller.hpp"
#include "[core_loader].hpp"

namespace Core::App::Render::Pages {

    class PagesServiceContainer: public BaseServiceContainer
    {
        Controller::Shared controller_ {};
    public:
        using Shared = std::shared_ptr<PagesServiceContainer>;

        explicit PagesServiceContainer(Controller::Shared controller): controller_(std::move(controller)) {}
        explicit PagesServiceContainer() = default;

        ~PagesServiceContainer() override = default;

        void SetupContainer(const PagesServiceContainer & parent)
        {
            controller_ = parent.controller_;

            BaseServiceContainer::SetupContainer(parent);
        }

        void SetupContainer(const PagesServiceContainer * parent)
        {
            SetupContainer(*parent);
        }

        [[nodiscard]] const Controller::Shared & GetController() const
        {
            return controller_;
        }
    };

    class PagesServiceInstance : public Utils::Service::Instance, public PagesServiceContainer {
    protected:
        // Game::Interface::Controller::Shared game_ {};
    public:
        using Shared = std::shared_ptr<PagesServiceInstance>;

        explicit PagesServiceInstance() = default;

        ~PagesServiceInstance() override = default;

        // void OnAllInterfacesLoaded() override
        // {
        //     game_ = IFace().Get<Game::Interface::Controller>();
        // }

        virtual void UpdateScene() = 0;

        virtual void HandleEvent(sf::Event & event, sf::RenderWindow & window) {}

        [[nodiscard]] virtual Game::MainState GetType() const = 0;

        [[nodiscard]] std::string GetServiceContainerName() const override
        {
            return Game::MainStateMap[GetType()];
        }
    };

    using Loader = Utils::Service::Loader;

    inline Loader & PagesLoader()
    {
        static Loader loader;
        return loader;
    }
} // namespace Core