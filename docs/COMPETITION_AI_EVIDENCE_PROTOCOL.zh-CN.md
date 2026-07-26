# PixelFlow Desk AI Competition Evidence Protocol

## Purpose

This protocol turns the device into a reproducible edge-AI demonstration rather than a collection of display effects. It is written for the ESP32-S3 intelligent-interaction AIoT direction: local sensing, local inference, interaction, connectivity, and service form one closed loop.

## What Runs on the ESP32-S3

The ESP32-S3 extracts five normalized features from the microphone, MPU6500, and LDR: audio activity, bass energy, motion change, ambient light, and engagement. A weighted nearest-prototype classifier produces Focus, Meeting, Rest, or Away locally. It does not upload raw audio or sensor data, and it does not require Wi-Fi for inference.

The personal profile is an online-learning model. App calibration labels update only the matching prototype. Blind-validation labels never update any prototype, so training data and evaluation data remain separate.

## Calibration Gate

Before validation, collect at least four calibration samples for every class. The App exposes three live indicators:

- Class coverage: how many of the four states have passed the four-sample minimum.
- Profile quality: combines sample coverage with prototype separation.
- Prototype separation: average weighted distance among calibrated class centers.

The profile is ready only after all four classes pass the sample threshold and their average separation is at least 0.12. Eight samples per class are recommended for the final recorded experiment.

## Blind Validation Procedure

1. Freeze calibration after the profile reaches the quality gate.
2. Place the device in one known desk state.
3. Wait two to three seconds for the inference stabilizer.
4. In the App, select the actual state under Validation evidence. Do not use the calibration buttons.
5. Repeat at least eight times per state under slightly varied but realistic conditions.
6. Record personalized accuracy, default-baseline accuracy, per-state recall, confusion matrix, inference latency, and offline inference count.

## Required Evidence for the Presentation

| Claim | Evidence shown live |
| --- | --- |
| Local Edge AI | local inference count and microsecond inference time |
| Multi-source sensing | five feature bars changing with sound, movement, and light |
| Personalization | per-class calibration samples stored in NVS after reboot |
| Fair model comparison | default baseline and personalized model use the same blind labels |
| Interpretability | current state, confidence, feature bars, and explanation text |
| Offline capability | disconnect the hotspot; the device keeps classifying and increments offline inference count |
| Service loop | the recognized state changes the display scene only when AI auto-scene is enabled |

## 90-Second Demonstration

1. Show the Training quality gate and the five live features.
2. Show four calibrated states and the personal profile becoming ready.
3. Switch to blind validation and label two or more independent samples.
4. Show personalized-versus-baseline accuracy and per-class recall.
5. Disable the hotspot, create a visible sound or movement change, and show the LED indicator still reacting.
6. Reconnect and show the accumulated offline-inference evidence in the App timeline.

## Claims to Avoid

Do not describe the device as identifying emotion, medical condition, personality, or an individual person. It only recognizes coarse desk-interaction states from local environmental signals.
