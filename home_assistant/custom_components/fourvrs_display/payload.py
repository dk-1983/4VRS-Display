"""Pure snapshot-v1 encoder, independently testable without Home Assistant."""
import json
import re

MAX_CARDS = 12
MAX_PAYLOAD = 9216
KINDS = {"fan", "light", "valve", "sensor", "binary_sensor", "switch"}


def validate_selection(entities):
    if not isinstance(entities, list) or not 1 <= len(entities) <= MAX_CARDS:
        raise ValueError("Select between one and twelve entities")
    if any(not isinstance(e, str) or len(e) > 96 or not re.fullmatch(r"[a-z][a-z0-9_]*\.[a-z0-9_]+", e) for e in entities):
        raise ValueError("Invalid entity id")
    if len(set(entities)) != len(entities):
        raise ValueError("Duplicate entity")
    return entities


def text(value, limit, default=""):
    if value is None:
        return default
    value = "".join(c for c in str(value) if ord(c) >= 32 and ord(c) != 127)
    return value.encode("utf-8", errors="replace")[:limit].decode("utf-8", errors="ignore") or default


def encode_snapshot(session, key, seq, entities, lookup, presenter=None, areas=None):
    validate_selection(entities)
    if not all(isinstance(v, str) and re.fullmatch(r"[0-9a-f]{32}", v) for v in (session, key)):
        raise ValueError("Invalid session or pairing key")
    if type(seq) is not int or not 1 <= seq <= 0xFFFFFFFF:
        raise ValueError("Sequence out of range")
    cards = []
    if areas is not None:
        # Sort using full names/IDs, never the truncated labels sent to the TFT.
        # Stable sorting preserves the user's selection order within each area.
        def area_key(entity_id):
            area_id, name = areas.get(entity_id, ("", ""))
            return (not bool(area_id), name.casefold(), area_id)
        entities = sorted(entities, key=area_key)
    for entity_id in entities:
        source = lookup(entity_id)
        attrs = source.attributes if source is not None else {}
        kind = entity_id.split(".", 1)[0]
        cards.append({
            "id": entity_id,
            "name": text(attrs.get("friendly_name"), 96, entity_id),
            "kind": kind if kind in KINDS else "text",
            "state": text(source.state if source is not None else "unavailable", 96, "unknown"),
            "unit": text(attrs.get("unit_of_measurement"), 16),
        })
        if areas is not None:
            cards[-1]["area"] = text(areas.get(entity_id, ("", ""))[1], 48)
        if presenter is not None:
            cards[-1].update(presenter(entity_id, attrs, cards[-1]["state"]))
    message = {"schema": 1, "key": key, "session": session, "seq": seq, "ttl_s": 90, "cards": cards}
    result = json.dumps(message, ensure_ascii=False, separators=(",", ":"), allow_nan=False)
    if len(result.encode("utf-8")) > MAX_PAYLOAD:
        raise ValueError("Payload too large")
    return result


def decode_capabilities(payload, device_id, request_id=None):
    if not isinstance(payload, str) or len(payload.encode("utf-8")) > MAX_PAYLOAD:
        raise ValueError("Invalid capabilities")
    data = json.loads(payload)
    if not isinstance(data, dict) or type(data.get("schema")) is not int or data.get("schema") != 1 or data.get("device_id") != device_id:
        raise ValueError("Wrong device or protocol")
    if not isinstance(data.get("session"), str) or not re.fullmatch(r"[0-9a-f]{32}", data["session"]):
        raise ValueError("Invalid session")
    if type(data.get("max_cards")) is not int or data["max_cards"] < MAX_CARDS:
        raise ValueError("Unsupported capacity")
    if request_id is not None and data.get("request_id") != request_id:
        raise ValueError("Unsolicited capabilities")
    return data
