idf_component_register(
    SRCS
        "fire_ai.cc"
        "ESP32_deploy/model/firegas_model.cc"
    INCLUDE_DIRS
        "include"
        "."                           # normalizer_params.h
        "ESP32_deploy/model"          # firegas_model.h
    REQUIRES
        fire_common
        espressif__esp-tflite-micro
)
