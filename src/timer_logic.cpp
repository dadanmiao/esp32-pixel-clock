/*
 * Author: Yang
 * Timer, countdown, and stopwatch state helpers.
 */
#include "timer_logic.h"

#include <cstring>

namespace {
uint32_t pomodoroDurationSec(const ControlState &control) {
  return (control.pomodoroIsBreak ? control.pomodoroBreakMin : control.pomodoroFocusMin) * 60UL;
}

void setIdleDuration(ControlState &control, uint32_t durationSec) {
  control.timerState = TimerRunState::Idle;
  control.timerDurationSec = durationSec;
  control.timerPausedRemainSec = durationSec;
  control.timerElapsedBeforePauseSec = 0;
  control.timerStartMillis = 0;
}
} // namespace

uint32_t timerRemainingSec(const ControlState &control, uint32_t nowMs) {
  if (control.timerMode == TimerMode::Stopwatch) {
    return 0;
  }
  if (control.timerState == TimerRunState::Idle) {
    return control.timerDurationSec;
  }
  if (control.timerState == TimerRunState::Paused) {
    return control.timerPausedRemainSec;
  }
  if (control.timerState == TimerRunState::Finished) {
    return 0;
  }

  const uint32_t elapsedSec = (nowMs - control.timerStartMillis) / 1000UL;
  if (elapsedSec >= control.timerDurationSec) {
    return 0;
  }
  return control.timerDurationSec - elapsedSec;
}

uint32_t stopwatchElapsedSec(const ControlState &control, uint32_t nowMs) {
  if (control.timerMode != TimerMode::Stopwatch || control.timerState == TimerRunState::Idle) {
    return 0;
  }
  if (control.timerState == TimerRunState::Running) {
    return control.timerElapsedBeforePauseSec + (nowMs - control.timerStartMillis) / 1000UL;
  }
  return control.timerElapsedBeforePauseSec;
}

void setTimerMode(ControlState &control, TimerMode mode) {
  control.timerMode = mode;
  control.timerStartMillis = 0;
  control.timerElapsedBeforePauseSec = 0;

  switch (mode) {
    case TimerMode::Countdown:
      if (control.timerDurationSec == 0) {
        control.timerDurationSec = AppConfig::DefaultCountdownMin * 60UL;
      }
      setIdleDuration(control, control.timerDurationSec);
      break;
    case TimerMode::Stopwatch:
      setIdleDuration(control, 0);
      break;
    case TimerMode::Pomodoro:
    default:
      control.pomodoroIsBreak = false;
      setIdleDuration(control, pomodoroDurationSec(control));
      break;
  }
}

void setCountdownMinutes(ControlState &control, uint16_t minutes) {
  minutes = constrain(minutes, static_cast<uint16_t>(1), static_cast<uint16_t>(99));
  control.timerMode = TimerMode::Countdown;
  setIdleDuration(control, minutes * 60UL);
}

void setPomodoroFocusMinutes(ControlState &control, uint16_t minutes) {
  control.pomodoroFocusMin = constrain(minutes, static_cast<uint16_t>(1), static_cast<uint16_t>(99));
  if (control.timerMode == TimerMode::Pomodoro && control.timerState == TimerRunState::Idle && !control.pomodoroIsBreak) {
    setIdleDuration(control, pomodoroDurationSec(control));
  }
}

void setPomodoroBreakMinutes(ControlState &control, uint16_t minutes) {
  control.pomodoroBreakMin = constrain(minutes, static_cast<uint16_t>(1), static_cast<uint16_t>(60));
  if (control.timerMode == TimerMode::Pomodoro && control.timerState == TimerRunState::Idle && control.pomodoroIsBreak) {
    setIdleDuration(control, pomodoroDurationSec(control));
  }
}

void startTimer(ControlState &control, uint32_t nowMs) {
  if (control.timerState == TimerRunState::Paused) {
    resumeTimer(control, nowMs);
    return;
  }

  control.timerStartMillis = nowMs;
  control.timerState = TimerRunState::Running;

  if (control.timerMode == TimerMode::Pomodoro) {
    control.timerDurationSec = pomodoroDurationSec(control);
    control.timerPausedRemainSec = control.timerDurationSec;
  } else if (control.timerMode == TimerMode::Countdown) {
    if (control.timerDurationSec == 0) {
      control.timerDurationSec = AppConfig::DefaultCountdownMin * 60UL;
    }
    control.timerPausedRemainSec = control.timerDurationSec;
  } else {
    control.timerDurationSec = 0;
    if (control.timerState != TimerRunState::Paused) {
      control.timerElapsedBeforePauseSec = 0;
    }
  }
}

void pauseTimer(ControlState &control, uint32_t nowMs) {
  if (control.timerState != TimerRunState::Running) {
    return;
  }

  if (control.timerMode == TimerMode::Stopwatch) {
    control.timerElapsedBeforePauseSec = stopwatchElapsedSec(control, nowMs);
  } else {
    control.timerPausedRemainSec = timerRemainingSec(control, nowMs);
  }
  control.timerState = TimerRunState::Paused;
}

void resumeTimer(ControlState &control, uint32_t nowMs) {
  if (control.timerState != TimerRunState::Paused) {
    return;
  }

  if (control.timerMode == TimerMode::Stopwatch) {
    control.timerStartMillis = nowMs;
  } else {
    control.timerDurationSec = control.timerPausedRemainSec;
    control.timerStartMillis = nowMs;
  }
  control.timerState = TimerRunState::Running;
}

void resetTimer(ControlState &control) {
  switch (control.timerMode) {
    case TimerMode::Countdown:
      setIdleDuration(control, control.timerDurationSec > 0 ? control.timerDurationSec : AppConfig::DefaultCountdownMin * 60UL);
      break;
    case TimerMode::Stopwatch:
      setIdleDuration(control, 0);
      break;
    case TimerMode::Pomodoro:
    default:
      control.pomodoroIsBreak = false;
      setIdleDuration(control, pomodoroDurationSec(control));
      break;
  }
}

void applyTimerAction(ControlState &control, const char *action, uint32_t nowMs) {
  if (!action) {
    return;
  }
  if (strcmp(action, "start") == 0) {
    startTimer(control, nowMs);
  } else if (strcmp(action, "pause") == 0) {
    pauseTimer(control, nowMs);
  } else if (strcmp(action, "resume") == 0) {
    resumeTimer(control, nowMs);
  } else if (strcmp(action, "reset") == 0) {
    resetTimer(control);
  } else if (strcmp(action, "toggle") == 0) {
    if (control.timerState == TimerRunState::Running) {
      pauseTimer(control, nowMs);
    } else if (control.timerState == TimerRunState::Paused) {
      resumeTimer(control, nowMs);
    } else {
      startTimer(control, nowMs);
    }
  }
}

bool serviceTimerState(uint32_t nowMs) {
  RenderState state = copySharedState();
  auto &control = state.control;
  if (control.timerState != TimerRunState::Running || control.timerMode == TimerMode::Stopwatch) {
    return false;
  }
  if (timerRemainingSec(control, nowMs) > 0) {
    return false;
  }

  control.timerState = TimerRunState::Finished;
  control.timerPausedRemainSec = 0;
  if (control.timerMode == TimerMode::Pomodoro) {
    control.pomodoroIsBreak = !control.pomodoroIsBreak;
    control.timerDurationSec = pomodoroDurationSec(control);
    control.timerPausedRemainSec = control.timerDurationSec;
  }
  updateControlState(control);
  pushRenderSnapshot(0);
  return true;
}
