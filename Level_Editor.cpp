// ============================================================================
// 2D Level Editor (Implementation)
// Programmed by Francois Lamini
// ============================================================================

#include "Level_Editor.h"

Codeloader::cLevel_Editor* editor = NULL;

bool On_Process();
bool On_Key_Process();
Codeloader::cArray<std::string> Get_Backgrounds(Codeloader::cAllegro_IO* allegro, int width, int height);
Codeloader::cArray<std::string> Get_Music_Tracks(Codeloader::cAllegro_IO* allegro);

// ****************************************************************************
// Program Entry Point
// ****************************************************************************

int main(int argc, char** argv) {
  try {
    Codeloader::cArray<std::string> param_names;
    param_names.Add("level");
    Codeloader::cParameters params(argc, argv, param_names);
    Codeloader::cConfig config("Config");
    int width = config.Get_Property("width");
    int height = config.Get_Property("height");
    Codeloader::cAllegro_IO allegro("Level Editor :: " + params["level"].string, width, height + 32, 2, "Game"); // Add space for HUD.
    allegro.Set_FPS(60); // Set frame rate!
    allegro.Load_Resources_From_Files();
    Codeloader::cArray<std::string> backgrounds = Get_Backgrounds(&allegro, width, height);
    Codeloader::cArray<std::string> music_tracks = Get_Music_Tracks(&allegro);
    editor = new Codeloader::cLevel_Editor(params["level"].string, config, &allegro, backgrounds, music_tracks);
    allegro.Process_Messages(On_Process, On_Key_Process);
    delete editor;
  }
  catch (Codeloader::cError error) {
    error.Print();
  }
  return 0;
}

/**
 * Processes the level editor loop. 
 */
bool On_Process() {
  editor->Process();
  return false;
}

/**
 * Processes a key to quit the editor. 
 */
bool On_Key_Process() {
  return false;
}

// ****************************************************************************
// Utility Routines
// ****************************************************************************

/**
 * Gets the list of backgrounds from the resources loaded.
 * @param allegro The Allegro I/O control.
 * @param width The width of the screen.
 * @param height The height of the screen.
 * @return The list of backgrounds.
 */
Codeloader::cArray<std::string> Get_Backgrounds(Codeloader::cAllegro_IO* allegro, int width, int height) {
  int image_count = allegro->images.Count(); 
  Codeloader::cArray<std::string> backgrounds;
  for (int image_index = 0; image_index < image_count; image_index++) {
    std::string image = allegro->images.keys[image_index];
    if (image.find("_Bkg") == (image.length() - 4)) {
      int bkg_width = allegro->Get_Image_Width(image);
      int bkg_height = allegro->Get_Image_Height(image);
      Codeloader::Check_Condition((bkg_width == width), "Background " + image + " is not the width of the screen.");
      Codeloader::Check_Condition((bkg_height == height), "Background " + image + " is not the height of the screen.");
      std::string background = image.substr(0, image.length() - 4);
      backgrounds.Add(background);
    }
  }
  return backgrounds;
}

/**
 * Gets the list of music track from the resources loaded.
 * @param allegro The Allegro I/O control.
 * @return The list of music tracks. 
 */
Codeloader::cArray<std::string> Get_Music_Tracks(Codeloader::cAllegro_IO* allegro) {
  Codeloader::cArray<std::string> music_tracks;
  int track_count = allegro->tracks.Count();
  for (int track_index = 0; track_index < track_count; track_index++) {
    music_tracks.Add(allegro->tracks.keys[track_index]);
  }
  return music_tracks;
}

// ****************************************************************************
// 2D Level Editor
// ****************************************************************************

namespace Codeloader {

  /**
   * Creates a new level editor.
   * @param name The name of the level to load.
   * @param config The config parser.
   * @param io The I/O control.
   * @param backgrounds The list of backgrounds provided to the editor.
   * @param music_tracks The list of music tracks provided to the editor.
   * @throws An error if the sprite palette could not be loaded.
   */
  cLevel_Editor::cLevel_Editor(std::string name, cConfig& config, cIO_Control* io, cArray<std::string> backgrounds, cArray<std::string> music_tracks) {
    this->io = io;
    this->level_name = name;
    this->sel_sprite = NO_VALUE_FOUND;
    this->sel_level = NO_VALUE_FOUND;
    this->scroll_x = 0;
    this->scroll_y = 0;
    this->timer = 0;
    this->backgrounds = backgrounds;
    Check_Condition((this->backgrounds.Count() > 0), "No backgrounds loaded!");
    this->background = this->backgrounds[0];
    this->music_tracks = music_tracks;
    Check_Condition((this->music_tracks.Count() > 0), "No music tracks defined.");
    this->music_track = this->music_tracks[0];
    cArray<std::string> layers = Parse_Sausage_Text(config.Get_Text_Property("layers"), ",");
    int layer_count = layers.Count();
    Check_Condition((layer_count > 0), "No layers defined!");
    for (int layer_index = 0; layer_index < layer_count; layer_index++) {
      std::string layer = layers[layer_index];
      this->layers[layer] = tObject_List();
    }
    this->sel_layer = layers[0]; // Set to lowest layer.
    std::string palette = config.Get_Text_Property("palette");
    this->Load_Sprite_Palette(palette);
    this->Load_Level(name);
  }

  /**
   * Frees the level editor. Any maintenance is done here. 
   */
  cLevel_Editor::~cLevel_Editor() {
    this->Save_Level(this->level_name);
  }

  /**
   * Loads the sprite palette.
   * @param name The name of the sprite palette.
   */
  void cLevel_Editor::Load_Sprite_Palette(std::string name) {
    cFile palette_file(name + ".txt");
    palette_file.Read();
    while (palette_file.Has_More_Lines()) {
      std::string sprite_name = palette_file.Get_Line();
      tObject sprite;
      palette_file >>= sprite;
      this->Destar_Sprite(sprite); // Important because properties can be starred from object catalog.
      Check_Condition(sprite.Does_Key_Exist("name"), "No sprite name present.");
      Check_Condition(sprite.Does_Key_Exist("layer"), "No layer present.");
      Check_Condition(sprite.Does_Key_Exist("x"), "No x coordinate.");
      Check_Condition(sprite.Does_Key_Exist("y"), "No y coordinate.");
      Check_Condition(sprite.Does_Key_Exist("size-x"), "No size x specifier.");
      Check_Condition(sprite.Does_Key_Exist("size-y"), "No size y specifier.");
      Check_Condition(sprite.Does_Key_Exist("icon"), "No icon present.");
      this->sprite_palette[sprite_name] = sprite;
    }
    Check_Condition((this->sprite_palette.Count() > 0), "No sprites in palette!");
    this->sel_sprite_type = this->sprite_palette.keys[0];
  }

  /**
   * Loads a level.
   * @param name The name of the level.
   */
  void cLevel_Editor::Load_Level(std::string name) {
    cFile level_file(name + ".map");
    try {
      level_file.Read();
      tObject meta_data;
      level_file >>= meta_data;
      Check_Condition(meta_data.Does_Key_Exist("background"), "No background property.");
      this->background = meta_data["background"].string;
      Check_Condition(meta_data.Does_Key_Exist("music-track"), "No music track property.");
      this->music_track = meta_data["music-track"].string;
      while (level_file.Has_More_Lines()) {
        tObject sprite;
        level_file >>= sprite;
        Check_Condition(sprite.Does_Key_Exist("name"), "No sprite name present.");
        Check_Condition(sprite.Does_Key_Exist("layer"), "No layer present.");
        Check_Condition(sprite.Does_Key_Exist("x"), "No x coordinate.");
        Check_Condition(sprite.Does_Key_Exist("y"), "No y coordinate.");
        Check_Condition(sprite.Does_Key_Exist("size-x"), "No size x specifier.");
        Check_Condition(sprite.Does_Key_Exist("size-y"), "No size y specifier.");
        Check_Condition(sprite.Does_Key_Exist("icon"), "No icon present.");
        std::string layer = sprite["layer"].string;
        Check_Condition(this->layers.Does_Key_Exist(layer), "Layer " + layer + " does not exist in layers.");
        this->layers[layer].Add(sprite);
      }
    }
    catch (cError error) {
      error.Print();
    }
  }

  /**
   * Saves a level to a file.
   * @param name The name of the level.
   */
  void cLevel_Editor::Save_Level(std::string name) {
    cFile level_file(name + ".map");
    tObject meta_data;
    meta_data["background"].Set_String(this->background);
    meta_data["music-track"].Set_String(this->music_track);
    level_file.Add(meta_data);
    int layer_count = this->layers.Count();
    for (int layer_index = 0; layer_index < layer_count; layer_index++) {
      std::string layer = this->layers.keys[layer_index];
      int sprite_count = this->layers[layer].Count();
      for (int sprite_index = 0; sprite_index < sprite_count; sprite_index++) {
        tObject& sprite = this->layers[layer][sprite_index];
        level_file.Add(sprite);
      }
    }
    level_file.Write();
  }

  /**
   * Handles the key processing in the editor. 
   */
  void cLevel_Editor::Process_Keys() {
    if (this->timer == 0) {
      sSignal key = this->io->Read_Key();
      if (this->sel_sprite == NO_VALUE_FOUND) {
        if (key.code == eSIGNAL_LEFT) {
          this->scroll_x -= 20;
          this->Set_Timer(); // Pause keyboard.
        }
        else if (key.code == eSIGNAL_RIGHT) {
          this->scroll_x += 20;
          this->Set_Timer();
        }
        if (key.code == eSIGNAL_UP) {
          this->scroll_y -= 20;
          this->Set_Timer();
        }
        else if (key.code == eSIGNAL_DOWN) {
          this->scroll_y += 20;
          this->Set_Timer();
        }
        // Select layer.
        if (key.code == 'z') {
          this->Select_Layer(-1);
          this->Set_Timer();
        }
        else if (key.code == 'x') {
          this->Select_Layer(1);
          this->Set_Timer();
        }
        // Select sprite type.
        if (key.code == 'c') {
          this->Select_Sprite_Type(-1);
          this->Set_Timer();
        }
        else if (key.code == 'v') {
          this->Select_Sprite_Type(1);
          this->Set_Timer();
        }
        // Select background.
        if (key.code == 'b') {
          this->Select_Background(-1);
          this->Set_Timer();
        }
        else if (key.code == 'n') {
          this->Select_Background(1);
          this->Set_Timer();
        }
        // Select music track.
        if (key.code == 'm') {
          this->Select_Music_Track(-1);
          this->io->Silence();
          this->Set_Timer();
        }
        else if (key.code == ',') {
          this->Select_Music_Track(1);
          this->io->Silence();
          this->Set_Timer();
        }
        // Play/stop music track.
        if (key.code == 'p') {
          this->io->Play_Music(this->music_track);
        }
        else if (key.code == 's') {
          this->io->Silence();
        }
      }
      else { // Sprite is selected.
        tObject& sprite = this->layers[this->sel_layer][this->sel_sprite];
        // Nudge sprite.
        if (key.code == eSIGNAL_LEFT) {
          sprite["x"].number--;
          this->Set_Timer();
        }
        else if (key.code == eSIGNAL_RIGHT) {
          sprite["x"].number++;
          this->Set_Timer();
        }
        if (key.code == eSIGNAL_UP) {
          sprite["y"].number--;
          this->Set_Timer();
        }
        else if (key.code == eSIGNAL_DOWN) {
          sprite["y"].number++;
          this->Set_Timer();
        }
        // Sizing of sprite.
        if (key.code == 'i') {
          if (sprite["size-y"].number > 1) {
            sprite["size-y"].number--;
          }
          this->Set_Timer();
        }
        else if (key.code == 'j') {
          if (sprite["size-x"].number > 1) {
            sprite["size-x"].number--;
          }
          this->Set_Timer();
        }
        else if (key.code == 'm') {
          sprite["size-y"].number++;
          this->Set_Timer();
        }
        else if (key.code == 'l') {
          sprite["size-x"].number++;
          this->Set_Timer();
        }
        // Deleting of sprite.
        if (key.code == eSIGNAL_DELETE) {
          this->layers[this->sel_layer].Remove(this->sel_sprite);
          this->sel_sprite = NO_VALUE_FOUND;
        }
        // Choosing sprite pointer level.
        if (sprite.Does_Key_Exist("pointer-level")) {
          cArray<std::string> level_list = this->Get_Level_List();
          int level_count = level_list.Count();
          int limit = level_count - 1;
          if (key.code == 'z') {
            this->sel_level--;
            if (this->sel_level < 0) {
              this->sel_level = limit;
            }
            sprite["pointer-level"].Set_String(level_list[this->sel_level]);
          }
          else if (key.code == 'x') {
            this->sel_level++;
            if (this->sel_level > limit) {
              this->sel_level = 0;
            }
            sprite["pointer-level"].Set_String(level_list[this->sel_level]);
          }
        }
      }
    }
  }

  /**
   * Processes the mouse for the editor. 
   */
  void cLevel_Editor::Process_Mouse() {
    sSignal mouse = this->io->Read_Signal();
    if (mouse.code == eSIGNAL_MOUSE) {
      if (mouse.button == eBUTTON_LEFT) {
        if (this->sel_sprite == NO_VALUE_FOUND) { // No sprite selected?
          this->sel_sprite = Select_Sprite(mouse.coords);
          if (this->sel_sprite == NO_VALUE_FOUND) {
            this->sel_sprite = this->Create_Sprite(mouse.coords); // Create a new sprite.
          }
          if (this->sel_sprite != NO_VALUE_FOUND) {
            tObject& sprite = this->layers[this->sel_layer][this->sel_sprite];
            if (sprite.Does_Key_Exist("pointer-level")) {
              this->sel_level = this->Find_Selected_Level_Index(sprite["pointer-level"].string);
              if (this->sel_level == NO_VALUE_FOUND) {
                this->sel_level = 0;
                sprite["pointer-level"].Set_String("None");
              }
            }
            else {
              this->sel_level = NO_VALUE_FOUND;
            }
          }
        }
      }
      else if (mouse.button == eBUTTON_RIGHT) {
        this->sel_sprite = NO_VALUE_FOUND;
      }
      else { // No button pressed.
        if (this->sel_sprite != NO_VALUE_FOUND) {
          tObject& sprite = this->layers[this->sel_layer][this->sel_sprite];
          sprite["x"].Set_Number(mouse.coords.x + this->scroll_x);
          sprite["y"].Set_Number(mouse.coords.y + this->scroll_y);
        }
      }
    }
  }

  /**
   * Sets the timer for the keyboard. 
   */
  void cLevel_Editor::Set_Timer() {
    this->timer = 60 / 5;
  }

  /**
   * Selects a layer given a direction.
   * @param direction The direction to select the layer from. 
   */
  void cLevel_Editor::Select_Layer(int direction) {
    int layer_count = this->layers.Count();
    for (int layer_index = 0; layer_index < layer_count; layer_index++) {
      std::string layer = this->layers.keys[layer_index];
      if (this->sel_layer == layer) {
        int next_layer = layer_index + direction;
        if (next_layer < 0) {
          next_layer = layer_count - 1;
        }
        if (next_layer == layer_count) {
          next_layer = 0;
        }
        this->sel_layer = this->layers.keys[next_layer];
        break;
      }
    }
  }

  /**
   * Determines if a sprite was select and which one was.
   * @param coords The coordinates of the mouse.
   * @return The index of the sprite selected or NO_VALUE_FOUND. 
   */
  int cLevel_Editor::Select_Sprite(sPoint coords) {
    int sel_sprite = NO_VALUE_FOUND;
    int sprite_count = this->layers[this->sel_layer].Count();
    for (int sprite_index = 0; sprite_index < sprite_count; sprite_index++) {
      tObject& sprite = this->layers[this->sel_layer][sprite_index];
      if (sprite["layer"].string == this->sel_layer) { // Make sure the sprite is in the correct layer.
        sRectangle bump_map;
        bump_map.left = sprite["x"].number - this->scroll_x;
        bump_map.top = sprite["y"].number - this->scroll_y;
        bump_map.right = bump_map.left + this->io->Get_Image_Width(sprite["icon"].string) - 1 - this->scroll_x;
        bump_map.bottom = bump_map.top + this->io->Get_Image_Height(sprite["icon"].string) - 1 - this->scroll_y;
        if (Is_Point_In_Box(coords, bump_map)) {
          sel_sprite = sprite_index;
          break;
        }
      }
    }
    return sel_sprite;
  }

  /**
   * Creates a new sprite.
   * @param coords The mouse coordinates where to lay down the sprite.
   * @return The index of the new sprite.
   */
  int cLevel_Editor::Create_Sprite(sPoint coords) {
    int sel_sprite = NO_VALUE_FOUND;
    tObject& sprite = this->sprite_palette[this->sel_sprite_type];
    if (sprite["layer"].string == this->sel_layer) {
      sprite["x"].Set_Number(this->scroll_x + coords.x);
      sprite["y"].Set_Number(this->scroll_y + coords.y);
      this->layers[this->sel_layer].Add(sprite);
      sel_sprite = this->layers[this->sel_layer].Count() - 1;
    }
    return sel_sprite;
  }

  /**
   * Selects a sprite type.
   * @param direction The direction to select the sprite in. 
   */
  void cLevel_Editor::Select_Sprite_Type(int direction) {
    int sprite_count = this->sprite_palette.Count();
    for (int sprite_index = 0; sprite_index < sprite_count; sprite_index++) {
      std::string sprite = this->sprite_palette.keys[sprite_index];
      if (this->sel_sprite_type == sprite) {
        int next_sprite = sprite_index + direction;
        if (next_sprite < 0) {
          next_sprite = sprite_count - 1;
        }
        if (next_sprite == sprite_count) {
          next_sprite = 0;
        }
        this->sel_sprite_type = this->sprite_palette.keys[next_sprite];
        break;
      }
    }
  }

  /**
   * Selects a background.
   * @param direction The direction to select the background in. 
   */
  void cLevel_Editor::Select_Background(int direction) {
    int background_count = this->backgrounds.Count();
    for (int background_index = 0; background_index < background_count; background_index++) {
      std::string background = this->backgrounds[background_index];
      if (this->background == background) {
        int next_background = background_index + direction;
        if (next_background < 0) {
          next_background = background_count - 1;
        }
        if (next_background == background_count) {
          next_background = 0;
        }
        this->background = this->backgrounds[next_background];
        break;
      }
    }
  }

  /**
   * Selects a music track.
   * @param direction The direction to select the music track in. 
   */
  void cLevel_Editor::Select_Music_Track(int direction) {
    int track_count = this->music_tracks.Count();
    for (int track_index = 0; track_index < track_count; track_index++) {
      std::string music_track = this->music_tracks[track_index];
      if (this->music_track == music_track) {
        int next_track = track_index + direction;
        if (next_track < 0) {
          next_track = track_count - 1;
        }
        if (next_track == track_count) {
          next_track = 0;
        }
        this->music_track = this->music_tracks[next_track];
        break;
      }
    }
  }

  /**
   * Renders the level.
   */
  void cLevel_Editor::Render() {
    // Color background white.
    this->io->Color(255, 255, 255);
    // Draw the background.
    int bkg_width = this->io->Get_Image_Width(this->background + "_Bkg");
    int bkg_height = this->io->Get_Image_Height(this->background + "_Bkg");
    this->io->Draw_Image(this->background + "_Bkg", 0, 0, bkg_width, bkg_height, 0, false, false);
    // Draw the sprites.
    int layer_count = this->layers.Count();
    for (int layer_index = 0; layer_index < layer_count; layer_index++) {
      std::string layer = this->layers.keys[layer_index];
      int sprite_count = this->layers[layer].Count();
      for (int sprite_index = 0; sprite_index < sprite_count; sprite_index++) {
        tObject& sprite = this->layers[layer][sprite_index];
        int x = sprite["x"].number;
        int y = sprite["y"].number;
        int size_x = sprite["size-x"].number;
        int size_y = sprite["size-y"].number;
        int sprite_width = this->io->Get_Image_Width(sprite["icon"].string);
        int sprite_height = this->io->Get_Image_Height(sprite["icon"].string);
        for (int sprite_y = 0; sprite_y < size_y; sprite_y++) {
          for (int sprite_x = 0; sprite_x < size_x; sprite_x++) {
            this->io->Draw_Image(sprite["icon"].string, x + (sprite_x * sprite_width) - this->scroll_x, y + (sprite_y * sprite_height) - this->scroll_y, sprite_width, sprite_height, 0, false, false);
          }
        }
      }
      // Render level console.
      this->io->Box(0, bkg_height, bkg_width, 32, 255, 255, 255); // Render white box.
      tObject& sprite_entry = this->sprite_palette[this->sel_sprite_type];
      this->io->Draw_Image(sprite_entry["icon"].string, 5, bkg_height + 5, 20, 20, 0, false, false);
      this->io->Output_Text(sprite_entry["layer"].string, 30, bkg_height + 3, 0, 0, 0); // Output the name of the layer that the sprite is on.
      int text_width = this->io->Get_Text_Width("Layer: " + this->sel_layer);
      int text_height = this->io->Get_Text_Height("Layer: " + this->sel_layer);
      this->io->Output_Text("Layer: " + this->sel_layer, bkg_width - 5 - text_width, bkg_height + 3, 0, 0, 0);
      if (this->sel_sprite != NO_VALUE_FOUND) {
        tObject& sprite = this->layers[this->sel_layer][this->sel_sprite];
        if (sprite.Does_Key_Exist("pointer-level")) {
          int x = sprite["x"].number;
          int y = sprite["y"].number;
          this->io->Output_Text("Points to: " + sprite["pointer-level"].string, x - this->scroll_x, y - text_height - this->scroll_y, 0, 255, 0);
        }
      }
      // Render debug log.
      while (this->debug_log.Count() > 0) {
        sDebug_Entry debug_entry = this->debug_log.Shift();
        int text_height = this->io->Get_Text_Height(debug_entry.text);
        this->io->Output_Text(debug_entry.text, debug_entry.x * text_height, debug_entry.y * text_height, 0, 0, 0);
      }
    }
  }

  /**
   * Processes the level editor. 
   */
  void cLevel_Editor::Process() {
    if (this->timer > 0) {
      this->timer--;
    }
    this->Process_Keys();
    this->Process_Mouse();
    this->Render();
    this->io->Refresh();
  }

  /**
   * Destars starred sprite properties.
   * @param sprite The sprite properties to destar.
   */
  void cLevel_Editor::Destar_Sprite(tObject& sprite) {
    int prop_count = sprite.Count();
    for (int prop_index = 0; prop_index < prop_count; prop_index++) {
      Check_Condition((sprite.keys[prop_index].length() > 0), "Key is NULL.");
      if (sprite.keys[prop_index][0] == '*') {
        sprite.keys[prop_index] = sprite.keys[prop_index].substr(1); // Destar the key.
      }
    }
  }

  /**
   * Prints an object.
   * @param object The object to print. 
   */
  void cLevel_Editor::Print_Object(tObject& object) {
    int prop_count = object.Count();
    for (int prop_index = 0; prop_index < prop_count; prop_index++) {
      std::string property = object.keys[prop_index];
      cValue& value = object[property];
      if (value.type == eVALUE_NUMBER) {
        std::cout << property << "=" << Number_To_Text(value.number) << std::endl;
      }
      else if (value.type == eVALUE_STRING) {
        std::cout << property << "=" << value.string << std::endl;
      }
    }
  }

  /**
   * Prints a debug statement on the screen.
   * @param text The text to print.
   * @param x The x coordinate. (Coordinates are in text coordinates.)
   * @param y The y coordinate.
   */
  void cLevel_Editor::Debug(std::string text, int x, int y) {
    sDebug_Entry debug_entry;
    debug_entry.text = text;
    debug_entry.x = x;
    debug_entry.y = y;
    this->debug_log.Add(debug_entry);
  }

  /**
   * Gets the list of levels.
   * @return The list of levels without the extension.
   */
  cArray<std::string> cLevel_Editor::Get_Level_List() {
    cArray<std::string> level_list;
    cArray<std::string> files = this->io->Get_File_List(this->io->Get_Current_Folder());
    int file_count = files.Count();
    level_list.Add("None"); // Add no level option.
    for (int file_index = 0; file_index < file_count; file_index++) {
      std::string file = files[file_index];
      std::string ext = this->io->Get_File_Extension(file);
      if (ext == "map") {
        std::string level = this->io->Get_File_Title(file);
        level_list.Add(level);
      }
    }
    return level_list;
  }

  /**
   * Finds the selected level index given the name.
   * @param name The name of the level.
   * @return The index of the level or NO_VALUE_FOUND if the level wasn't found.
   */
  int cLevel_Editor::Find_Selected_Level_Index(std::string name) {
    int selected_level = NO_VALUE_FOUND;
    cArray<std::string> level_list = this->Get_Level_List();
    int level_count = level_list.Count();
    for (int level_index = 0; level_index < level_count; level_index++) {
      std::string level = level_list[level_index];
      if (level == name) {
        selected_level = level_index;
        break;
      }
    }
    return selected_level;
  }

}
