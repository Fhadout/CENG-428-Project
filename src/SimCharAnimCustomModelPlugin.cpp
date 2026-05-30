// Arkheon Simulation Technologies
// Proprietary and Confidential.
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// © Arkheon Simulation Technologies. All rights reserved.

#include "SimCharAnimCustomModelPlugin.h"

#include <model/AnimationModel.h>
#include <model/IModel.h>
#include <model/ModelFactoryRegistry.h>
#include <plugin/IModelPluginService.h>
#include <plugin/PluginContext.h>
#include <plugin/IPluginServices.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <unordered_set>

namespace arkheon::sample::simcharanimcustommodel {
namespace {

class CustomAnimationModel final
    : public arkheon::astsim::IModel,
      public arkheon::astsim::IAnimationModel {
public:
    [[nodiscard]] std::string getTypeName() const override {
        return "animationModelCustom";
    }

    [[nodiscard]] std::unique_ptr<arkheon::astsim::IModel> clone() const override {
        return std::make_unique<CustomAnimationModel>(*this);
    }

    [[nodiscard]] bool evaluate(
        const arkheon::astsim::AnimationModelInput& input,
        arkheon::astsim::AnimationModelOutput& output) override {

        if (input.entity.activeAnimationCode != "Idle Neutral") {
            return false;
        }

        std::unordered_set<std::string> availableJointIds;
        availableJointIds.reserve(input.entity.joints.size());

        for (const auto& joint : input.entity.joints) {
            availableJointIds.insert(joint.jointId);
        }

        const double t = input.simulationTimeSeconds;
        const double wave = std::sin(t * 1.2);

        output.clearExistingJointOverrides = true;
        output.jointOverrides.clear();

        auto addJoint = [&](
            const char* jointId,
            double x,
            double y,
            double z) {

            if (hasJoint(availableJointIds, jointId)) {
                output.jointOverrides.push_back({jointId, x, y, z});
            }
        };

        // 1. Left ankle
        addJoint("leftAnkle", -0.15 * wave, 0.0, 0.0);

        // 2. Right ankle
        addJoint("rightAnkle", 0.15 * wave, 0.0, 0.0);

        // 3. Left knee
        addJoint("leftKnee", 0.35 * std::max(0.0, -wave), 0.0, 0.0);

        // 4. Right knee
        addJoint("rightKnee", 0.35 * std::max(0.0, wave), 0.0, 0.0);

        // 5. Left hip
        addJoint("leftHip", 0.25 * wave, 0.0, 0.0);

        // 6. Right hip
        addJoint("rightHip", -0.25 * wave, 0.0, 0.0);

        // 7. Left shoulder
        addJoint("leftShoulder", -0.30 * wave, 0.0, -0.10);

        // 8. Right shoulder
        addJoint("rightShoulder", 0.30 * wave, 0.0, 0.10);

        // 9. Left elbow
        addJoint("leftElbow", 0.20 + 0.15 * std::max(0.0, wave), 0.0, 0.0);

        // 10. Right elbow
        addJoint("rightElbow", 0.20 + 0.15 * std::max(0.0, -wave), 0.0, 0.0);

        return !output.jointOverrides.empty();
    }

private:
    [[nodiscard]] static bool hasJoint(
        const std::unordered_set<std::string>& availableJointIds,
        const char* jointId) {

        if (!jointId || *jointId == '\0') {
            return false;
        }

        if (availableJointIds.empty()) {
            return true;
        }

        return availableJointIds.find(jointId) != availableJointIds.end();
    }
};

} // namespace

int SimCharAnimCustomModelPlugin::getInterfaceVersion() const {
    return 1;
}

arkheon::astlib::PluginMetadata SimCharAnimCustomModelPlugin::getMetadata() const {
    arkheon::astlib::PluginMetadata metadata;
    metadata.setPluginId("sim-char-anim-custom-model");
    metadata.setVersion("1.0.0");
    metadata.setAuthor("Arkheon Sample");
    return metadata;
}

void SimCharAnimCustomModelPlugin::initialize(
    arkheon::astlib::PluginContext& context) {

    initialized_ = true;
    shutdown_ = false;
    modelRegistered_ = false;
    modelType_ = "animationModelCustom";

    modelFactoryRegistry_ = nullptr;

    if (context.services) {
        auto* rawService =
            context.services->getService(
                arkheon::astsim::IModelPluginService::kPluginServiceId);

        auto* service =
            static_cast<arkheon::astsim::IModelPluginService*>(rawService);

        modelFactoryRegistry_ =
            service ? &service->modelFactoryRegistry() : nullptr;
    }

    if (!modelFactoryRegistry_) {
        return;
    }

    modelRegistered_ =
        modelFactoryRegistry_->registerFactory(
            modelType_,
            std::make_unique<CustomAnimationModel>());
}

void SimCharAnimCustomModelPlugin::tick(double dt) {
    static_cast<void>(dt);

    if (!initialized_ || shutdown_) {
        return;
    }
}

void SimCharAnimCustomModelPlugin::shutdown() {
    if (modelFactoryRegistry_) {
        if (modelRegistered_) {
            static_cast<void>(
                modelFactoryRegistry_->unregisterFactory(modelType_));
        }
    }

    modelRegistered_ = false;
    shutdown_ = true;
    modelFactoryRegistry_ = nullptr;
}

} // namespace arkheon::sample::simcharanimcustommodel

extern "C" {

ARKHEON_ASTLIB_API arkheon::astlib::IPlugin* create_plugin() {
    return new arkheon::sample::simcharanimcustommodel::
        SimCharAnimCustomModelPlugin();
}

ARKHEON_ASTLIB_API void destroy_plugin(arkheon::astlib::IPlugin* plugin) {
    if (plugin) {
        delete plugin;
    }
}

ARKHEON_ASTLIB_API const char* get_plugin_signature() {
    return "ARKHEON_PLUGIN_V1";
}

} // extern "C"