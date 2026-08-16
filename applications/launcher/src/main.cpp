//
// Created by kapil on 12.02.2026.
//

#include "binary_file.h"
#include "downloader.h"
#include "logger.h"

#include "launcher.h"

int main() {
    logger::init("JAPILauncher", "japi/logs");

    try {
        launcher{}
			.run();
    } catch (const std::exception& e) {
        ERROR_AND_QUIT(e.what());
    }

    JINFO("Shutting down launcher...");
}
