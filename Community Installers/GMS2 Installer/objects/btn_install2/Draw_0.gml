/// @DnDAction : YoYo Games.Drawing.Draw_Self
/// @DnDVersion : 1
/// @DnDHash : 550693C8
draw_self();

/// @DnDAction : YoYo Games.Common.If_Variable
/// @DnDVersion : 1
/// @DnDHash : 3AC69C32
/// @DnDArgument : "var" "hovering"
/// @DnDArgument : "value" "1"
if(hovering == 1)
{
	/// @DnDAction : YoYo Games.Instances.Set_Sprite
	/// @DnDVersion : 1
	/// @DnDHash : 6BB05B1E
	/// @DnDParent : 3AC69C32
	/// @DnDArgument : "spriteind" "in2h"
	/// @DnDSaveInfo : "spriteind" "in2h"
	sprite_index = in2h;
	image_index = 0;
}

/// @DnDAction : YoYo Games.Common.If_Variable
/// @DnDVersion : 1
/// @DnDHash : 4B40F444
/// @DnDArgument : "var" "hovering"
if(hovering == 0)
{
	/// @DnDAction : YoYo Games.Instances.Set_Sprite
	/// @DnDVersion : 1
	/// @DnDHash : 40B0587C
	/// @DnDParent : 4B40F444
	/// @DnDArgument : "spriteind" "in2"
	/// @DnDSaveInfo : "spriteind" "in2"
	sprite_index = in2;
	image_index = 0;
}