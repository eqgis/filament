/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef VIEWER_AUTOMATION_ENGINE_H
#define VIEWER_AUTOMATION_ENGINE_H

#include <viewer/AutomationSpec.h>

namespace filament {

class MaterialInstance;
class Renderer;
class View;

namespace viewer {

/**
 * Provides a convenient way to iterate through an AutomationSpec while pushing settings to Filament
 * and exporting screenshots.
 *
 * Upon construction, the automation engine is given an immutable reference to an AutomationSpec.
 * The engine is always in one of two states: running or idle. The running state can be entered
 * either immediately (startRunning) or by requesting batch mode (startBatchMode).
 *
 * Clients must call tick() after each frame is rendered, which gives the engine an opportunity to
 * push settings to Filament, increment the current test index (if enough time has elapsed), and
 * request an asychronous screenshot.
 *
 * The time to sleep between tests is configurable and can be set to zero. The engine also waits a
 * specified minimum number of frames between tests.
 *
 * Batch mode is meant for non-interactive applications. In batch mode, the engine defers applying
 * the first test case until the client unblocks it via signalBatchMode(). This is useful when
 * waiting for a large model file to become fully loaded. Batch mode also offers a query
 * (shouldClose) that is triggered after the last screenshot has been written to disk.
 */
class AutomationEngine {
public:
    /**
     * Allows users to toggle screenshots, change the sleep duration between tests, etc.
     */
    struct Options {
        /**
         * Minimum time that the engine waits between applying a settings object and subsequently
         * taking a screenshot. After the screenshot is taken, the engine immediately advances to
         * the next test case. Specified in seconds.
         */
        float sleepDuration = 0.2;

        /**
         * Similar to sleepDuration, but expressed as a frame count. Both the minimum sleep time
         * and the minimum frame count must be elapsed before the engine advances to the next test.
         */
        int minFrameCount = 2;

        /**
         * If true, test progress is dumped to the utils Log (info priority).
         */
        bool verbose = true;

        /**
         * If true, the tick function writes out a screenshot before advancing to the next test.
         */
        bool exportScreenshots = false;

        /**
         * If true, the tick function writes out a settings JSON file before advancing.
         */
        bool exportSettings = false;
    };

    /**
     * Creates an automation engine and places it in an idle state.
     *
     * @param spec     Specifies a set of settings permutations (owned by the client).
     * @param settings Client-owned settings object. This not only supplies the initial
     *                 state, it also receives changes during tick(). This is useful when
     *                 building automation into an application that has a settings UI.
     *
     * @see setOptions
     * @see startRunning
     */
    AutomationEngine(const AutomationSpec* spec, Settings* settings) :
            mSpec(spec), mSettings(settings) {}

    /**
     * Shortcut constructor that creates an automation engine from a JSON string.
     *
     * This constructor can be used if the user does not need to monitor how the settings
     * change over time and does not need ownership over the AutomationSpec.
     *
     * @param jsonSpec Valid JSON string that conforms to the automation schema.
     * @param size     Number of characters in the JSON string.
     * @return         Automation engine or null if unable to read the JSON.
     */
    static AutomationEngine* createFromJSON(const char* jsonSpec, size_t size);

    /**
     * Creates an automation engine for the default test sequence.
     *
     * To see how the default test sequence is generated, search for DEFAULT_AUTOMATION.
     */
    static AutomationEngine* createDefaultTest();

    ~AutomationEngine();

    /**
     * Activates automation. During the subsequent call to tick(), the first test is applied
     * and the engine enters the running state.
     */
    void startRunning();

    /**
     * Activates automation, but enters a paused state until the user calls signalBatchMode().
     */
    void startBatchMode();

    /**
     * Notifies the automation engine that time has passed and a new frame has been rendered.
     *
     * This is when settings get applied, screenshots are (optionally) exported, etc.
     *
     * @param view          The Filament View that automation pushes changes to.
     * @param materials     An optional set of of materials that can receive parameter tweaks.
     * @param materialCount The number of items in the materials array.
     * @param renderer      The Filament Renderer that can be used to take screenshots.
     * @param deltaTime     The amount of time that has passed since the previous tick in seconds.
     */
    void tick(View* view, MaterialInstance* const* materials, size_t materialCount,
            Renderer* renderer, float deltaTime);

    /**
     * Signals that batch mode can begin. Call this after all meshes and textures finish loading.
     */
    void signalBatchMode() { mBatchModeAllowed = true; }

    /**
     * Cancels an in-progress automation session.
     */
    void stopRunning() { mIsRunning = false; }

    /**
     * Signals that the application is closing, so all pending screenshots should be cancelled.
     */
    void terminate();

    /**
     * Configures the automation engine for users who wish to set up a custom sleep time
     * between tests, etc.
     */
    void setOptions(Options options) { mOptions = options; }

    /**
     * Returns true if automation is in batch mode and all tests have finished.
     */
    bool shouldClose() const { return mShouldClose; }

    /**
     * Convenience function that writes out a JSON file to disk containing all settings.
     *
     * @param Settings State vector to serialize.
     * @param filename Desired JSON filename.
     */
    static void exportSettings(const Settings& settings, const char* filename);

    Options getOptions() const { return mOptions; }
    bool isRunning() const { return mIsRunning; }
    size_t currentTest() const { return mCurrentTest; }
    size_t testCount() const { return mSpec->size(); }
    bool isBatchModeEnabled() const { return mBatchModeEnabled; }
    const char* getStatusMessage() const;

private:
    AutomationSpec const * const mSpec;
    Settings * const mSettings;
    Options mOptions;
    size_t mCurrentTest;
    float mElapsedTime;
    int mElapsedFrames;
    bool mIsRunning = false;
    bool mBatchModeEnabled = false;
    bool mRequestStart = false;
    bool mShouldClose = false;
    bool mBatchModeAllowed = false;
    bool mTerminated = false;
    bool mOwnsSettings = false;

public:
    // For internal use from a screenshot callback.
    void requestClose() { mShouldClose = true; }
    bool isTerminated() const { return mTerminated; }
};

} // namespace viewer
} // namespace filament

#endif // VIEWER_AUTOMATION_ENGINE_H
