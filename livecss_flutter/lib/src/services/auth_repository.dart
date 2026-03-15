import 'dart:convert';

import '../models/auth_models.dart';
import 'app_storage.dart';
import 'app_storage_base.dart';
import 'api_client.dart';

class AuthRepository {
  AuthRepository({
    required ApiClient apiClient,
    AppStorage? storage,
  })  : _apiClient = apiClient,
        _storage = storage ?? createAppStorage();

  static const String _sessionKey = 'xcm_auth_session';

  final ApiClient _apiClient;
  final AppStorage _storage;

  Future<LoginData> login({
    required String identifier,
    required String password,
  }) async {
    final envelope = await _apiClient.post(
      '/auth/login',
      body: {
        'identifier': identifier,
        'password': password,
      },
    );

    final data = (envelope.data ?? <String, dynamic>{}) as Map<String, dynamic>;
    return LoginData.fromJson(data);
  }

  Future<AuthSession> verifyTwoFA({
    required String challengeToken,
    required String code,
  }) async {
    final envelope = await _apiClient.post(
      '/auth/verify-2fa',
      bearer: challengeToken,
      body: {'code': code},
    );

    final payload = (envelope.data ?? <String, dynamic>{}) as Map<String, dynamic>;
    final user = payload['user'] is Map<String, dynamic>
        ? SafeUser.fromJson(payload['user'] as Map<String, dynamic>)
        : await me(challengeToken);
    final tokens = TokenPair.fromJson(
      (payload['tokens'] ?? <String, dynamic>{}) as Map<String, dynamic>,
    );
    final session = AuthSession(tokens: tokens, user: user);
    await saveSession(session);
    return session;
  }

  Future<void> resendTwoFA(String challengeToken) async {
    await _apiClient.post(
      '/auth/resend-2fa',
      bearer: challengeToken,
      body: const <String, dynamic>{},
    );
  }

  Future<AuthSession> createSessionFromLogin(LoginData loginData) async {
    final user = loginData.user;
    final tokens = loginData.tokens;
    if (user == null || tokens == null) {
      throw ApiException('Login response did not include session tokens');
    }

    final session = AuthSession(tokens: tokens, user: user);
    await saveSession(session);
    return session;
  }

  Future<void> saveSession(AuthSession session) async {
    await _storage.write(key: _sessionKey, value: jsonEncode(session.toJson()));
  }

  Future<AuthSession?> loadSession() async {
    final raw = await _storage.read(key: _sessionKey);
    if (raw == null || raw.isEmpty) {
      return null;
    }

    final decoded = jsonDecode(raw);
    if (decoded is! Map<String, dynamic>) {
      return null;
    }

    return AuthSession.fromJson(decoded);
  }

  Future<void> clearSession() async {
    await _storage.delete(key: _sessionKey);
  }

  Future<SafeUser> me(String accessToken) async {
    final envelope = await _apiClient.get('/user/me', bearer: accessToken);
    return SafeUser.fromJson((envelope.data ?? <String, dynamic>{}) as Map<String, dynamic>);
  }

  Future<AuthSession> refresh(AuthSession session) async {
    final envelope = await _apiClient.post(
      '/auth/refresh',
      body: {'refresh_token': session.tokens.refreshToken},
    );
    final data = (envelope.data ?? <String, dynamic>{}) as Map<String, dynamic>;
    final refreshedTokens = TokenPair.fromJson(data);
    final refreshedUser = await me(refreshedTokens.accessToken);
    final refreshedSession = AuthSession(tokens: refreshedTokens, user: refreshedUser);
    await saveSession(refreshedSession);
    return refreshedSession;
  }

  Future<void> logout(AuthSession session) async {
    try {
      await _apiClient.post(
        '/auth/logout',
        bearer: session.tokens.accessToken,
        body: {'refresh_token': session.tokens.refreshToken},
      );
    } finally {
      await clearSession();
    }
  }
}
