import 'dart:convert';
import 'package:google_generative_ai/google_generative_ai.dart';
import '../models/heptapod_spectrum.dart';
import '../models/spectrum_summary.dart';

class GeminiService {
  final GenerativeModel _model;

  GeminiService(String apiKey)
      : _model = GenerativeModel(
          model: 'gemini-1.5-flash',
          apiKey: apiKey,
        );

  // Constructor for testing injection
  GeminiService.withModel(this._model);

  Future<HeptapodSpectrum> textToSpectrum(String text) async {
    final systemPrompt = '''
Analyze this text for a Heptapod visual language. Break it into 3 Frequency Layers.
* **Core Layer:** Main theme (Low Freq 2-6, High Amp).
* **Narrative Layer:** Events (Mid Freq 8-20, Mid Amp).
* **Nuance Layer:** Emotion (High Freq 20-60, Low Amp).
* **Chaos Factor:** If the text is angry/confused, set high chaos (0.8). If calm, low chaos (0.1).

Return ONLY JSON:
{
  'core': [{'freq': 3.0, 'amp': 50.0, 'chaosFactor': 0.2}],
  'narrative': [...],
  'nuance': [...]
}
''';

    final prompt = '$systemPrompt\n\nInput Text:\n$text';
    final content = [Content.text(prompt)];
    final response = await _model.generateContent(content);

    if (response.text == null) {
      throw Exception('Gemini returned empty response');
    }

    return parseSpectrumJson(response.text!);
  }

  Future<String> spectrumToPhilosophy(SpectrumSummary summary) async {
    final systemPrompt = '''
I am sending you data extracted from a Heptapod Logogram.
* **Dominant Frequencies:** ${summary.dominantFrequencies} (Shape complexity).
* **Chaos Level (0-1):** ${summary.chaosLevel} (0 = Smooth/Peaceful, 1 = Jagged/Violent/Confused).
* **Ink Density:** ${summary.density} (Heaviness of the concept).

**Task:** Interpret the philosophical meaning.
* If Chaos is high, describe inner conflict, entropy, or energy.
* If Chaos is low, describe harmony, silence, or flow.
* If Density is high, describe heavy, grounding concepts (Time, Gravity, Death).
* If Density is low, describe ethereal concepts (Dream, Ghost, Whisper).

Output: A poetic, timeless interpretation in Turkish. Do not mention the numbers.
''';

    final content = [Content.text(systemPrompt)];
    final response = await _model.generateContent(content);

    if (response.text == null) {
      throw Exception('Gemini returned empty response for philosophy');
    }

    return response.text!;
  }

  HeptapodSpectrum parseSpectrumJson(String jsonStr) {
    // Remove markdown code blocks if present
    String cleanedJson = jsonStr.replaceAll(RegExp(r'```json|```'), '').trim();
    // Sometimes keys in the example are single quoted, but standard JSON is double quoted.
    // The prompt used single quotes in the example, so Gemini might return single quotes.
    // Standard dart:convert jsonDecode expects double quotes.
    // However, Gemini usually returns valid JSON (double quotes).
    // If it returns single quotes, we might have issues.
    // Let's assume standard JSON but clean up potential formatting issues.

    // Also handling potentially ' mapped keys if necessary, but risky to replace all ' with ".
    // Let's rely on Gemini being smart enough to return valid JSON or standard JSON.
    // The prompt example used single quotes: {'core': ...} which is technically not valid JSON standard (double quotes required).
    // I should probably update the prompt to use double quotes to be safe, but the instructions gave single quotes.
    // I will try to parse. If it fails, I might need to replace ' with " for keys.

    try {
      final Map<String, dynamic> jsonData = jsonDecode(cleanedJson);
      return HeptapodSpectrum.fromJson(jsonData);
    } catch (e) {
      // If standard decode fails, try to fix single quotes
      try {
        final fixedJson = cleanedJson.replaceAll("'", '"');
        final Map<String, dynamic> jsonData = jsonDecode(fixedJson);
        return HeptapodSpectrum.fromJson(jsonData);
      } catch (e2) {
         throw FormatException('Failed to parse Gemini JSON response: $e\nOriginal: $jsonStr');
      }
    }
  }
}
