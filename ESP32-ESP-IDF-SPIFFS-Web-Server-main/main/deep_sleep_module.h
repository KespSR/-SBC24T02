// deep_sleep_module.h

#ifndef DEEP_SLEEP_MODULE_H
#define DEEP_SLEEP_MODULE_H

#include <stdint.h>

/**
 * @brief Inicializa el módulo de deep sleep.
 * Configura las fuentes de despertado (por GPIO en este caso).
 */
void deep_sleep_module_init(void);

/**
 * @brief Entra en modo de deep sleep.
 */
void deep_sleep_module_enter(void);

#endif // DEEP_SLEEP_MODULE_H
