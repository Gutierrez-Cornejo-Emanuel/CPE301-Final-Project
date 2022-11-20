//from //lab 4, example 3
void write_to_pin(volatile unsigned char* port_address, unsigned char pin_num, unsigned char state) {
  if(state == 0)
  {
    *port_address &= ~(0x01 << pin_num);
  }
  else
  {
    *port_address |= 0x01 << pin_num;
  }
}
//from lab 4, example 3
void set_pin_as_output(volatile unsigned char* ddr_n, unsigned char pin_num)
{
    *ddr_n |= 0x01 << pin_num;
}