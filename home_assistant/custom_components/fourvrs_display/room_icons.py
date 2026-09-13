"""Small room icon vocabulary; names remain unchanged on the display."""
import re

# More specific rooms precede generic room names. English words match whole words;
# Russian stems allow ordinary endings (спальня / спальне / спальная).
RULES = [
    ("toilet", r"туалет|сануз|\b(wc|toilet|restroom)\b"),
    ("bathroom", r"ванн|душев|\b(bath|bathroom|shower)\b"),
    ("kitchen", r"кухн|столов|\b(kitchen|dining)\b"),
    ("kids", r"детск|игров|\b(kids|nursery|playroom)\b"),
    ("bedroom", r"спаль|\b(bedroom|bed)\b"),
    ("living", r"гостин|\b(living|lounge)\b"),
    ("garage", r"гараж|мастерск|\b(garage|workshop)\b"),
    ("server", r"сервер|\b(server|rack)\b"),
    ("office", r"кабинет|офис|\b(office|study)\b"),
    ("balcony", r"балкон|лоджи|террас|\b(balcony|terrace|loggia)\b"),
    ("hall", r"прихож|коридор|холл|\b(hall|hallway|corridor|entrance)\b"),
    ("garden", r"сад|двор|улиц|\b(garden|yard|outdoor)\b"),
    ("laundry", r"прачеч|постир|\b(laundry|utility)\b"),
    ("storage", r"кладов|склад|подвал|\b(storage|basement|cellar)\b"),
]
ICON_NAMES = {name for name, _ in RULES} | {"room"}

def room_icon(name):
    name = name.casefold()
    return next((icon for icon, pattern in RULES if re.search(pattern, name)), "room")
