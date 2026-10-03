#pragma once

/* Initializes positions, directions, colors, dimensions, and runtime speeds. */
void Game_Init(void);
/* Runs one gameplay frame: input, speed adjustment, AI, movement, and drawing. */
void Game_Update(void);
/* State shutdown hook. No owned resources currently require release. */
void Game_Exit(void);
