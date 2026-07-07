#ifndef BIT_OPERATIONS_H
#define BIT_OPERATIONS_H

#define SET_BIT(REG, BIT) (REG |= (0X01 << BIT))
#define CLEAR_BIT(REG, BIT) (REG &= ~(0X01 << BIT))
#define READ_BIT(REG, BIT) (REG & (0X01 << BIT))
#define TOGGLE_BIT(REG, BIT) (REG ^= (0X01 << BIT))

#endif