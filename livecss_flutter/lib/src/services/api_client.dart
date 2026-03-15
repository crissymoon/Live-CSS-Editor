import 'dart:convert';
import 'dart:io';

import 'package:http/http.dart' as http;

import '../models/auth_models.dart';

const String _authBaseOverride =
    String.fromEnvironment('XCM_AUTH_URL', defaultValue: '');

String resolveAuthBaseUrl() {
  if (_authBaseOverride.isNotEmpty) {
    return _authBaseOverride;
  }
  if (Platform.isAndroid) {
    return 'http://10.0.2.2:9100';
  }
  return 'http://127.0.0.1:9100';
}

class ApiException implements Exception {
  ApiException(this.message, {this.statusCode});

  final String message;
  final int? statusCode;

  @override
  String toString() => message;
}

class ApiClient {
  ApiClient({http.Client? httpClient}) : _httpClient = httpClient ?? http.Client();

  final http.Client _httpClient;

  Future<ApiEnvelope<dynamic>> post(
    String path, {
    Map<String, dynamic>? body,
    String? bearer,
  }) async {
    final uri = Uri.parse('${resolveAuthBaseUrl()}$path');
    final response = await _httpClient.post(
      uri,
      headers: _headers(bearer),
      body: jsonEncode(body ?? <String, dynamic>{}),
    );
    return _decodeEnvelope(response);
  }

  Future<ApiEnvelope<dynamic>> get(
    String path, {
    String? bearer,
  }) async {
    final uri = Uri.parse('${resolveAuthBaseUrl()}$path');
    final response = await _httpClient.get(uri, headers: _headers(bearer));
    return _decodeEnvelope(response);
  }

  Map<String, String> _headers(String? bearer) {
    final headers = <String, String>{
      'Content-Type': 'application/json',
      'Accept': 'application/json',
    };
    if (bearer != null && bearer.isNotEmpty) {
      headers['Authorization'] = 'Bearer $bearer';
    }
    return headers;
  }

  ApiEnvelope<dynamic> _decodeEnvelope(http.Response response) {
    final dynamic decoded = jsonDecode(response.body);
    if (decoded is! Map<String, dynamic>) {
      throw ApiException('Invalid server response', statusCode: response.statusCode);
    }

    final envelope = ApiEnvelope<dynamic>.fromJson(decoded, (raw) => raw);
    if (!envelope.ok || response.statusCode >= 400) {
      final fallback = envelope.message ?? 'Request failed (${response.statusCode})';
      final errors = envelope.errors;
      if (errors != null && errors.isNotEmpty) {
        throw ApiException(errors.join('\n'), statusCode: response.statusCode);
      }
      throw ApiException(fallback, statusCode: response.statusCode);
    }
    return envelope;
  }
}
