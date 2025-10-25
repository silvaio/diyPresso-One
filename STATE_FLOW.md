# diyPresso-One State Flow Diagram

This document shows the complete state flow for the diyPresso-One espresso machine, including both the Brew Process and Boiler Control state machines.

## 🎯 Overview

The diyPresso-One uses two main state machines:
- **Brew Process**: Controls the overall machine operation and brewing sequence
- **Boiler Control**: Manages boiler temperature and heating states

## 📊 Brew Process State Machine

### Main States

```mermaid
stateDiagram-v2
    [*] --> init : Power On
    
    init --> idle : Commissioned + Button Press
    init --> fill : Not Commissioned + Button Press
    
    fill --> purge : Pump Timeout (30s) + Weight Drop ≥ 125g
    fill --> error : Pump Timeout (30s) + Weight Drop < 125g
    
    purge --> check : Purge Timeout (30s) + Weight Drop ≥ 50g
    purge --> error : Purge Timeout (30s) + Weight Drop < 50g
    
    check --> done : Commissioning Complete
    done --> idle : Brew Switch Down
    
    idle --> warning_pre_brew : Brew Switch Up + Almost Empty
    idle --> pre_infuse : Brew Switch Up + Reservoir OK
    idle --> sleep : Auto-sleep Timeout (1 hour)
    idle --> empty : Reservoir Empty
    idle --> init : Not Commissioned
    
    warning_pre_brew --> idle : Brew Switch Down
    warning_pre_brew --> pre_infuse : Button Press
    
    pre_infuse --> infuse : Pre-infuse Timeout (3s)
    infuse --> extract : Infuse Timeout (4s)
    extract --> finished : Extract Timeout (10s)
    
    finished --> extract : Button Press (Re-extract)
    finished --> idle : Finished Timeout (60s)
    
    sleep --> shutdown : Sleep Timeout (4 hours)
    sleep --> idle : Wake Up Message
    
    shutdown --> idle : Button Press (Wake Up)
    
    empty --> idle : Reservoir Refilled + Brew Switch Down
    
    error --> init : Reset Message
    
    note right of init
        Initial state at startup
        - If commissioned: go to idle
        - If not commissioned: wait for button
    end note
    
    note right of fill
        Fill boiler with water
        - Pump for 30 seconds
        - Check weight drop ≥ 125g
        - Safety verification
    end note
    
    note right of sleep
        Sleep mode
        - Check boiler level before sleep
        - Optional temperature maintenance
        - Auto-shutdown after 4 hours
    end note
    
    note right of shutdown
        Complete shutdown
        - All systems off
        - Red LED indicator
        - Button press to wake up
    end note
```

### State Details

| State | LED Color | Description | Triggers |
|-------|-----------|-------------|----------|
| `init` | BLACK | Initial startup state | Power on |
| `fill` | BLACK | Filling boiler during commissioning | Button press in init |
| `purge` | BLACK | Purging air from boiler | Successful fill |
| `check` | BLACK | Checking boiler temperature | Successful purge |
| `done` | BLACK | Commissioning complete | Temperature stable |
| `idle` | GREEN | Ready to brew | Normal operation |
| `warning_pre_brew` | RED | Low water warning | Almost empty reservoir |
| `pre_infuse` | BLUE | Pre-infusion phase | Start brewing |
| `infuse` | YELLOW | Infusion/soaking phase | Pre-infuse complete |
| `extract` | PURPLE | Extraction phase | Infusion complete |
| `finished` | CYAN | Brewing complete | Extraction complete |
| `sleep` | BLACK | Sleep mode | 1 hour inactivity |
| `shutdown` | RED | Complete shutdown | 4 hours in sleep |
| `empty` | CYAN | Reservoir empty | No water detected |
| `error` | RED | Error state | Various errors |

## 🔥 Boiler Control State Machine

### Boiler States

```mermaid
stateDiagram-v2
    [*] --> off : Initial State
    
    off --> heating : Boiler On
    heating --> ready : Temperature Reached
    heating --> brew : Brew Started
    heating --> off : Boiler Off
    
    ready --> heating : Temperature Drop
    ready --> brew : Brew Started
    ready --> off : Boiler Off
    
    brew --> heating : Brew Stopped
    brew --> off : Boiler Off
    
    off --> error : Error Condition
    heating --> error : Error Condition
    ready --> error : Error Condition
    brew --> error : Error Condition
    
    error --> off : Error Cleared
    
    note right of off
        Boiler completely off
        - No heating
        - No power consumption
    end note
    
    note right of heating
        Heating to target temperature
        - PID control active
        - Feed-forward enabled
        - Temperature rising
    end note
    
    note right of ready
        At target temperature
        - PID control active
        - Feed-forward enabled
        - Temperature stable
    end note
    
    note right of brew
        Brewing mode
        - Higher feed-forward
        - Optimized for extraction
        - Temperature maintained
    end note
```

### Boiler State Details

| State | Description | Feed Forward | PID Control |
|-------|-------------|--------------|-------------|
| `off` | Boiler completely off | 0% | Disabled |
| `heating` | Heating to target | Heat FF | Active |
| `ready` | At target temperature | Ready FF | Active |
| `brew` | Brewing mode | Brew FF | Active |
| `error` | Error state | 0% | Disabled |

## 🔄 State Transitions & Triggers

### Brew Process Triggers

| Trigger | Description | States Affected |
|---------|-------------|-----------------|
| **Button Press** | Dial button pressed | `init` → `fill`, `warning_pre_brew` → `pre_infuse`, `finished` → `extract`, `shutdown` → `idle` |
| **Brew Switch Up** | Switch moved to brew position | `idle` → `pre_infuse`/`warning_pre_brew` |
| **Brew Switch Down** | Switch moved to idle position | `pre_infuse`/`infuse`/`extract` → `idle` |
| **Auto-sleep Timeout** | 1 hour of inactivity | `idle` → `sleep` |
| **Sleep Timeout** | 4 hours in sleep | `sleep` → `shutdown` |
| **Wake Up Message** | Long button press | `sleep` → `idle` |
| **Reservoir Empty** | No water detected | Any state → `empty` |
| **Reservoir Refilled** | Water detected | `empty` → `idle` |
| **Commissioning Complete** | Initial setup done | `check` → `done` |
| **Reset Message** | Error cleared | `error` → `init` |

### Boiler Control Triggers

| Trigger | Description | States Affected |
|---------|-------------|-----------------|
| **Boiler On** | `boilerController.on()` | `off` → `heating` |
| **Boiler Off** | `boilerController.off()` | Any state → `off` |
| **Start Brew** | `boilerController.start_brew()` | `heating`/`ready` → `brew` |
| **Stop Brew** | `boilerController.stop_brew()` | `brew` → `heating` |
| **Temperature Reached** | Within temp window | `heating` → `ready` |
| **Temperature Drop** | Outside temp window | `ready` → `heating` |
| **Error Condition** | Various errors | Any state → `error` |
| **Error Cleared** | Error reset | `error` → `off` |

## 🛡️ Safety Features

### Boiler Level Checking

The system automatically checks and refills the boiler at critical times:

| Check Type | Trigger | Description |
|------------|---------|-------------|
| **Startup Check** | After power-on | Verify boiler level after startup |
| **Pre-sleep Check** | Before entering sleep | Ensure boiler is full before sleep |
| **Post-brew Check** | After brewing | Replenish water lost during brewing |
| **Emergency Check** | Dry boiler detected | Automatic refill when dry boiler detected |

### Safety Timeouts

| Timeout | Duration | Action |
|---------|----------|--------|
| **Auto-sleep** | 1 hour | Enter sleep mode |
| **Shutdown** | 4 hours | Complete system shutdown |
| **Boiler Fill** | 60 seconds | Maximum fill time (safety) |
| **Boiler Fill Test** | 5 seconds | Check cycle duration |

## 🎮 User Interactions

### Button Functions

| Action | Function | States Affected |
|--------|----------|-----------------|
| **Short Press** | Menu navigation, state transitions | Various states |
| **Long Press** | Sleep/Wake toggle | `idle` ↔ `sleep` |
| **Press in Shutdown** | Wake up from shutdown | `shutdown` → `idle` |

### Brew Switch Functions

| Position | Function | States Affected |
|----------|----------|-----------------|
| **UP (Open)** | Start brewing | `idle` → `pre_infuse` |
| **DOWN (Closed)** | Stop brewing, return to idle | Brewing states → `idle` |

## 📈 State Flow Summary

### Normal Operation Flow
```
Power On → Init → Idle → Pre-infuse → Infuse → Extract → Finished → Idle
```

### Sleep Flow
```
Idle → Sleep (1h) → Shutdown (4h) → [Button Press] → Idle
```

### Commissioning Flow
```
Power On → Init → Fill → Purge → Check → Done → Idle
```

### Error Recovery Flow
```
Any State → Error → [Reset] → Init → Idle
```

This state flow ensures safe, reliable operation with automatic safety features and intuitive user interactions.



