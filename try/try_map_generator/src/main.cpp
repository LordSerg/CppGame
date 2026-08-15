#include "app.h"

int main() {
    App app;

    if (!app.init(1280, 720, "Map Generator")) {
        return -1;
    }

    app.run();
    app.shutdown();

    return 0;
}