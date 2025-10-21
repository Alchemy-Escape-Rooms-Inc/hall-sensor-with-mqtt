idf_component_register(SRCS
    "main.cpp"
    "hall_sensor_logic/SensorManager.cpp"
    "hall_sensor_logic/SensorWrapper.cpp"
    "hall_sensor_logic/Trigger.cpp"
    "hall_sensor_logic/PuzzleState.cpp"
    "peripherals/pins.cpp"
    "peripherals/i2c.cpp"
    
    INCLUDE_DIRS "" "hall_sensor_logic" "peripherals" "allegro"
)
