#include "cprocessing.h"
#include "mainmenu.h"
#include "game.h"
#include "utils.h"
#include <math.h>

/* ================= HẰNG SỐ (chỉnh ở đây để đổi độ khó của NPC) ================= */
#define CHARACTER_RADIUS 30.0f   /* bán kính của mỗi nhân vật                       */
#define MOVE_SPEED       300.0f  /* tốc độ di chuyển (pixel / giây)                 */
#define EDGE_MARGIN      5.0f    /* chừa lề 5px quanh cửa sổ (giống code cũ)        */
#define NPC_LOOKAHEAD    15.0f   /* NPC "nhìn trước" 15px để thử bước đi kế tiếp    */
#define NPC_AVOID_DIST   160.0f  /* trong vòng 160px quanh player, NPC không chịu đi lại gần thêm */

/* ================= Global variables ================= */
float dt;

CP_Vector direction_up;
CP_Vector direction_down;
CP_Vector direction_left;
CP_Vector direction_right;

/* Nhân vật đang được ĐIỀU KHIỂN (player) */
CP_Vector character1;
CP_Vector directionForPlayer;
CP_Color color1;
int rotation;

/* Nhân vật KHÔNG được điều khiển (NPC, do AI lái) */
CP_Vector character2;
CP_Vector directionForNpc;
CP_Color color2;
int rotationNpc;

CP_Color colorDir;   /* màu trắng của hình tam giác */

float width;         /* chú ý: đây là CHIỀU CAO cửa sổ (giữ nguyên tên cũ của bạn) */
float length;        /* chú ý: đây là CHIỀU RỘNG cửa sổ */


/* ================= HÀM HỖ TRỢ ================= */

/*
 * Đổi số thứ tự hướng (0..3) thành vector hướng.
 * Quy ước này TRÙNG với góc xoay của tam giác (góc = số thứ tự * 90):
 *   0 -> phải (0 độ)   1 -> xuống (90 độ)   2 -> trái (180 độ)   3 -> lên (270 độ)
 */
static CP_Vector get_direction(int dirIndex)
{
	if (dirIndex == 0)
	{
		return direction_right;
	}
	if (dirIndex == 1) 
	{
		return direction_down;
	}
	if (dirIndex == 2) 
	{
		return direction_left;
	}
	return direction_up;
}

/* ================= PLAYER (giữ nguyên logic cũ) ================= */

static CP_Vector draw_movement_for_player(CP_Vector direction, CP_Vector character)
{
	if (CP_Vector_Length(direction) > 0.0f)
	{
		direction = CP_Vector_Normalize(direction);
		direction = CP_Vector_Scale(direction, MOVE_SPEED);
	}

	character = CP_Vector_Add(character, CP_Vector_Scale(direction, dt));
	return character;
}

static CP_Vector direction_for_player(CP_KEY keyLeft, CP_KEY keyRight, CP_KEY keyUp, CP_KEY keyDown)
{
	CP_Vector direction = CP_Vector_Zero();

	if (CP_Input_KeyDown(keyLeft))
	{
		direction = direction_left;
		rotation = 180;
	}
	else if (CP_Input_KeyDown(keyRight))
	{
		direction = direction_right;
		rotation = 0;
	}
	else if (CP_Input_KeyDown(keyUp))
	{
		direction = direction_up;
		rotation = 270;
	}
	else if (CP_Input_KeyDown(keyDown))
	{
		direction = direction_down;
		rotation = 90;
	}
	return direction;
}

/* ================= NPC AI (PHẦN MỚI) ================= */

/*
 * NPC tự hỏi: "Nếu mình đi thêm một bước theo hướng dir thì có ổn không?"
 * Trả về 1 nếu ổn, 0 nếu không ổn. Hai điều kiện "không ổn":
 *   (a) bước tiếp theo làm NPC đâm vào biên cửa sổ
 *   (b) NPC đang ở gần player mà bước tiếp theo lại tiến sát player hơn nữa
 */
static int is_direction_valid(float r, CP_Vector npc, CP_Vector dir, CP_Vector player)
{
	/* Vị trí NPC sẽ đứng nếu bước thử theo hướng dir */
	CP_Vector next = CP_Vector_Add(npc, CP_Vector_Scale(dir, NPC_LOOKAHEAD));

	/* (a) Kiểm tra biên: chỉ cấm hướng đang đi VỀ PHÍA bức tường mà bước sau sẽ chạm */
	if (dir.x > 0.0f && next.x + r > length - EDGE_MARGIN) return 0;  /* tường phải */
	if (dir.x < 0.0f && next.x - r < EDGE_MARGIN)          return 0;  /* tường trái */
	if (dir.y > 0.0f && next.y + r > width - EDGE_MARGIN)  return 0;  /* tường dưới */
	if (dir.y < 0.0f && next.y - r < EDGE_MARGIN)          return 0;  /* tường trên */

	/* (b) Kiểm tra player: trong vùng nguy hiểm (< NPC_AVOID_DIST) chỉ cho phép bước đi
	 * làm khoảng cách TĂNG LÊN (chạy ra xa). Bước đi làm khoảng cách giảm hoặc giữ nguyên bị cấm. */
	float distanceNow = CP_Vector_Distance(npc, player);
	float distanceAfter = CP_Vector_Distance(next, player);
	if (distanceAfter < NPC_AVOID_DIST && distanceAfter <= distanceNow)
	{
		return 0;
	}
	return 1;
}

/*
 * Bộ não của NPC: mỗi frame quyết định NPC đi hướng nào.
 * Trả về vector hướng (hoặc vector 0 nếu NPC phải đứng yên) và cập nhật rotationNpc.
 */
static CP_Vector direction_for_npc(float r, CP_Vector npc, CP_Vector currentDirection, CP_Vector player)
{
	int isMoving = CP_Vector_Length(currentDirection) > 0.0f;

	/* BƯỚC 1: Hướng hiện tại vẫn ổn => cứ thế đi tiếp, không đổi hướng
	 * (giúp NPC đi mượt, không bị "rung" đổi hướng liên tục) */
	if (isMoving && is_direction_valid(r, npc, currentDirection, player))
	{
		return currentDirection;
	}

	/* BƯỚC 2: Thử cả 4 hướng, trong các hướng "ổn" chọn hướng XA player nhất */
	int bestIndex = -1;          /* -1 nghĩa là "chưa tìm được hướng nào" */
	float bestDistance = -1.0f;

	for (int i = 0; i < 4; i++)
	{
		CP_Vector candidate = get_direction(i);

		if (is_direction_valid(r, npc, candidate, player))
		{
			CP_Vector next = CP_Vector_Add(npc, CP_Vector_Scale(candidate, NPC_LOOKAHEAD));
			float d = CP_Vector_Distance(next, player);

			if (d > bestDistance)
			{
				bestDistance = d;
				bestIndex = i;
			}
		}
	}

	/* BƯỚC 3: Không có hướng nào ổn (kẹt góc) => đứng yên, giữ nguyên hướng nhìn */
	if (bestIndex == -1)
	{
		return CP_Vector_Zero();
	}

	/* Có hướng tốt nhất => quay mặt về hướng đó */
	rotationNpc = bestIndex * 90;
	return get_direction(bestIndex);
}

static CP_Vector movement_for_npc(CP_Vector npc, CP_Vector direction, float speed)
{
	npc = CP_Vector_Add(npc, CP_Vector_Scale(direction, speed * dt));
	return npc;
}

/* ================= ĐỔI NHÂN VẬT KHI CLICK (PHẦN MỚI) ================= */

/*
 * Hoán đổi "vai trò" của 2 nhân vật: character1 luôn là nhân vật đang điều khiển,
 * character2 luôn là nhân vật do AI lái. Vì vậy khi đổi, ta đổi chỗ toàn bộ dữ liệu.
 */
static void swap_characters(void)
{
	CP_Vector tempPosition = character1;
	character1 = character2;
	character2 = tempPosition;

	CP_Color tempColor = color1;
	color1 = color2;
	color2 = tempColor;

	int tempRotation = rotation;
	rotation = rotationNpc;
	rotationNpc = tempRotation;

	/* NPC mới tiếp tục đi theo hướng nó đang nhìn, AI sẽ tự điều chỉnh ở frame sau */
	directionForNpc = get_direction(rotationNpc / 90);
}

/* ================= VẼ ================= */

/* Vẽ 1 nhân vật = hình tròn + tam giác trắng chỉ hướng (dùng chung cho cả player và NPC) */
static void drawNpc(CP_Color color, CP_Vector npc, float r, int rotationAngle)
{
	CP_Settings_NoStroke();
	CP_Settings_Fill(color);
	CP_Graphics_DrawCircle(npc.x, npc.y, r * 2);
	CP_Settings_Fill(colorDir);
	CP_Graphics_DrawTriangleAdvanced(npc.x + r, npc.y, npc.x - r / 2, npc.y - r * sqrtf(3.0f) / 2, npc.x - r / 2, npc.y + r * sqrtf(3.0f) / 2, (float)rotationAngle);
}

/* ================= CÁC HÀM CỦA STATE ================= */

void Game_Init(void)
{
	dt = CP_System_GetDt();
	rotation = 0;
	length = (float)CP_System_GetWindowWidth();
	width = (float)CP_System_GetWindowHeight();

	direction_down = CP_Vector_Set(0.0f, 1.0f);
	direction_up = CP_Vector_Set(0.0f, -1.0f);
	direction_left = CP_Vector_Set(-1.0f, 0.0f);
	direction_right = CP_Vector_Set(1.0f, 0.0f);

	character1 = CP_Vector_Set(100.0f, 200.0f);
	color1 = CP_Color_Create(0, 0, 190, 255);
	colorDir = CP_Color_Create(255, 255, 255, 255);

	character2 = CP_Vector_Set(300.0f, 400.0f);
	color2 = CP_Color_Create(200, 0, 0, 255);
	rotationNpc = 90;                                  /* nhìn xuống */
	directionForNpc = CP_Vector_Set(0.0f, 1.0f);       /* và đi xuống (khớp với rotationNpc) */
}

void Game_Update(void)
{
	dt = CP_System_GetDt();

	CP_Graphics_ClearBackground(CP_Color_Create(245, 240, 176, 200));

	/* --- 1. Click chuột để đổi nhân vật (PHẦN MỚI) --- */
	if (CP_Input_MouseTriggered(MOUSE_BUTTON_LEFT))
	{
		float mouseX = CP_Input_GetMouseX();
		float mouseY = CP_Input_GetMouseY();

		/* Chỉ đổi khi click trúng NPC, và không click trúng player đang nằm TRÊN (vì player vẽ đè lên) */
		if (IsCircleClicked(character2.x, character2.y, CHARACTER_RADIUS * 2, mouseX, mouseY) && !IsCircleClicked(character1.x, character1.y, CHARACTER_RADIUS * 2, mouseX, mouseY))
		{
			swap_characters();
		}
	}

	/* --- 2. Cập nhật vị trí (player trước, rồi NPC nhìn vị trí mới của player để phản ứng) --- */
	directionForPlayer = direction_for_player(KEY_A, KEY_D, KEY_W, KEY_S);
	character1 = draw_movement_for_player(directionForPlayer, character1);

	directionForNpc = direction_for_npc(CHARACTER_RADIUS, character2, directionForNpc, character1);
	character2 = movement_for_npc(character2, directionForNpc, MOVE_SPEED);

	/* --- 3. Vẽ: NPC vẽ TRƯỚC (nằm dưới), player vẽ SAU (nằm trên) --- */
	drawNpc(color2, character2, CHARACTER_RADIUS, rotationNpc);
	drawNpc(color1, character1, CHARACTER_RADIUS, rotation);

	if (CP_Input_KeyTriggered(KEY_Q))
	{
		CP_Engine_SetNextGameState(Main_Menu_Init, Main_Menu_Update, Main_Menu_Exit);
	}
}

void Game_Exit(void)
{

}