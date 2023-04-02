#include <string>
#include <variant>
#include <vector>
#include <stack>
namespace json
{

  struct JsonNode
  {
  };

  struct JsonValue: public JsonNode
  {
  };
  struct JsonObject: public JsonNode
  {
    using Attribute = struct {
      std::string name;
      JsonNode*   value;
    };

    std::vector<Attribute> attributes;
  };
  struct JsonArray: public JsonNode
  {
  };


  struct ObjectParsingInfo
  {
    int field_index;
    int location; // 0: before anything, 1: while parsing name, 2: before parsing value, 3: parsing value
  };


  struct Json
  {
    char* buffer;
    JsonNode* node;

    JsonNode* parsing_node;
  };



  using ParsingInfo = std::variant<ObjectParsingInfo>;
  using ParsingStack = std::stack<ParsingInfo>;

  inline void Trim(std::string& s)
  {
    int i = 0;
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() != '\n'))
      i++;
    s.erase(0, i);
  }

  int ReadNext(ParsingStack& parsing_stack, std::string buffer)
  {
    if (parsing_stack.empty())
    {
      Trim(buffer);
      if (!buffer.empty() && buffer.front() == '{')
      {
        buffer.erase(0, 1);
        Trim(buffer);
        ObjectParsingInfo info;
        if (buffer.em
      }
      else if (!buffer.empty() && buffer.front() == '[')
      {
      }
      else
      {
        return 0;
      }
    }
    else
    {
      ParsingInfo pinfo = parsing_stack.top();
      std::visit([&buffer, &parsing_stack] (auto&& info) {
        using T = std::decay_t<decltype(info)>;
        if constexpr (std::is_same_v<ObjectParsingInfo, T>)
        {
        }
        else
        {
          assert(false);
        }
      }, pinfo);
    }
    return 0;
  }
}


int main() {
  json::ParsingStack stack;
  ReadNext(stack, "{ \"Hello\": ");
}
