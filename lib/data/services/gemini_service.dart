import 'dart:convert';
import 'package:google_generative_ai/google_generative_ai.dart';
import '../models/heptapod_spectrum.dart';

class GeminiService {
  final GenerativeModel _model;

  GeminiService(String apiKey)
      : _model = GenerativeModel(
          model: 'gemini-pro',
          apiKey: apiKey,
        );

  Future<HeptapodSpectrum> textToSpectrum(String text) async {
    const systemPrompt = """
Analyze this text. Extract its essence into 3 Frequency Layers.
* Layer 1 (Core): Main theme (Low Freq 1-5, High Amp).
* Layer 2 (Narrative): Events/Action (Mid Freq 6-15).
* Layer 3 (Nuance): Emotion/Atmosphere (High Freq 16-50, Low Amp).
Return ONLY a JSON object with this structure:
{
  'core': [{'freq': 3.5, 'amp': 40.0, 'shape': 'sine'}],
  'narrative': [...],
  'nuance': [...]
}
""";

    final content = [Content.text('$systemPrompt\n\nInput Text: "$text"')];

    try {
      final response = await _model.generateContent(content);

      if (response.text == null) {
        throw Exception('No response from Gemini');
      }

      // Clean the response text to ensure it's valid JSON
      // sometimes models output markdown code blocks
      String jsonString = response.text!;
      if (jsonString.contains('```json')) {
        jsonString = jsonString.split('```json')[1].split('```')[0].trim();
      } else if (jsonString.contains('```')) {
        jsonString = jsonString.split('```')[1].split('```')[0].trim();
      }

      final Map<String, dynamic> jsonMap = jsonDecode(jsonString);
      return HeptapodSpectrum.fromJson(jsonMap);
    } catch (e) {
      throw Exception('Failed to generate spectrum: $e');
    }
  }
}
