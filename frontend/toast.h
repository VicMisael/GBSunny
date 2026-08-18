#pragma once

#include <string>

namespace frontend
{
    class Toast final
    {
    public:
        void show(std::string title, std::string message, double duration = 3.5);
        void draw(float top) const;

    private:
        std::string title;
        std::string message;
        double shown_at = 0.0;
        double duration = 0.0;
    };
}
