// ============================================================================
// 2D Level Editor (Definitions)
// Programmed by Francois Lamini
// ============================================================================

#include "..\Code_Helper\Codeloader.hpp"
#include "..\Code_Helper\Allegro.hpp"

namespace Codeloader {

  struct sDebug_Entry {
    std::string text;
    int x;
    int y;
  };

  class cLevel_Editor {

    public:
      cHash<std::string, tObject_List> layers;
      std::string sel_layer;
      int sel_sprite;
      std::string music_track;
      std::string background;
      cHash<std::string, tObject> sprite_palette;
      std::string sel_sprite_type;
      int sel_level;
      std::string level_name;
      int scroll_x;
      int scroll_y;
      int timer;
      cIO_Control* io;
      cArray<std::string> backgrounds;
      cArray<std::string> music_tracks;
      cArray<sDebug_Entry> debug_log;

      cLevel_Editor(std::string name, cConfig& config, cIO_Control* io, cArray<std::string> backgrounds, cArray<std::string> music_tracks);
      ~cLevel_Editor();
      void Load_Sprite_Palette(std::string name);
      void Load_Level(std::string name);
      void Save_Level(std::string name);
      void Process_Keys();
      void Process_Mouse();
      void Set_Timer();
      void Select_Layer(int direction);
      int Select_Sprite(sPoint coords);
      int Create_Sprite(sPoint coords);
      void Select_Sprite_Type(int direction);
      void Select_Background(int direction);
      void Select_Music_Track(int direction);
      void Render();
      void Process();
      void Destar_Sprite(tObject& sprite);
      void Print_Object(tObject& object);
      void Debug(std::string text, int x, int y);
      cArray<std::string> Get_Level_List();
      int Find_Selected_Level_Index(std::string name);
  
  };

}