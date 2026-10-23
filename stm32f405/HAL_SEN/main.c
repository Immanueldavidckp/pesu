int main()
	{
	HAL_Init()
	SystemClock_Confi
	
	while(1)
		{
		h1_state=HALGPIO_ReadPin(
		h2_state
		h3_state=HAL_GPIO_ReadPin(
		// hall code = 4*h1 + 2*h2 + h3
		hall_code = (h1_state <<2) | (h2_state <<1) | h3_state ;
		HAL_Delay(10);
		
