/// @DnDAction : YoYo Games.Drawing.Draw_Self
/// @DnDVersion : 1
/// @DnDHash : 7F85223A
draw_self();

/// @DnDAction : YoYo Games.Common.If_Variable
/// @DnDVersion : 1
/// @DnDHash : 1FE2A5C6
/// @DnDArgument : "var" "global.version"
/// @DnDArgument : "value" "11"
if(global.version == 11)
{
	/// @DnDAction : YoYo Games.Instances.Set_Sprite
	/// @DnDVersion : 1
	/// @DnDHash : 77D7D836
	/// @DnDParent : 1FE2A5C6
	/// @DnDArgument : "spriteind" "button10un"
	/// @DnDSaveInfo : "spriteind" "button10un"
	sprite_index = button10un;
	image_index = 0;
}

/// @DnDAction : YoYo Games.Common.If_Variable
/// @DnDVersion : 1
/// @DnDHash : 450B2AFB
/// @DnDArgument : "var" "global.version"
/// @DnDArgument : "value" "10"
if(global.version == 10)
{
	/// @DnDAction : YoYo Games.Instances.Set_Sprite
	/// @DnDVersion : 1
	/// @DnDHash : 43E432DB
	/// @DnDParent : 450B2AFB
	/// @DnDArgument : "spriteind" "button10"
	/// @DnDSaveInfo : "spriteind" "button10"
	sprite_index = button10;
	image_index = 0;
}