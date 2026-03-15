class ApiEnvelope<T> {
  const ApiEnvelope({required this.ok, this.message, this.data, this.errors});

  final bool ok;
  final String? message;
  final T? data;
  final List<String>? errors;

  factory ApiEnvelope.fromJson(
    Map<String, dynamic> json,
    T Function(dynamic raw)? parseData,
  ) {
    final rawErrors = json['errors'];
    return ApiEnvelope<T>(
      ok: json['ok'] == true,
      message: json['message'] as String?,
      data: parseData != null ? parseData(json['data']) : null,
      errors: rawErrors is List
          ? rawErrors.whereType<String>().toList()
          : null,
    );
  }
}

class TokenPair {
  const TokenPair({required this.accessToken, required this.refreshToken});

  final String accessToken;
  final String refreshToken;

  factory TokenPair.fromJson(Map<String, dynamic> json) {
    return TokenPair(
      accessToken: (json['access_token'] ?? '') as String,
      refreshToken: (json['refresh_token'] ?? '') as String,
    );
  }

  Map<String, dynamic> toJson() => {
        'access_token': accessToken,
        'refresh_token': refreshToken,
      };
}

class SafeUser {
  const SafeUser({
    required this.id,
    required this.username,
    required this.email,
    required this.role,
    required this.isActive,
    required this.isVerified,
  });

  final int id;
  final String username;
  final String email;
  final String role;
  final bool isActive;
  final bool isVerified;

  factory SafeUser.fromJson(Map<String, dynamic> json) {
    return SafeUser(
      id: (json['id'] as num? ?? 0).toInt(),
      username: (json['username'] ?? '') as String,
      email: (json['email'] ?? '') as String,
      role: (json['role'] ?? 'user') as String,
      isActive: json['is_active'] == true,
      isVerified: json['is_verified'] == true,
    );
  }
}

class LoginData {
  const LoginData({
    required this.twoFARequired,
    this.challengeToken,
    this.user,
    this.tokens,
  });

  final bool twoFARequired;
  final String? challengeToken;
  final SafeUser? user;
  final TokenPair? tokens;

  factory LoginData.fromJson(Map<String, dynamic> json) {
    return LoginData(
      twoFARequired: json['twofa_required'] == true,
      challengeToken: json['challenge_token'] as String?,
      user: json['user'] is Map<String, dynamic>
          ? SafeUser.fromJson(json['user'] as Map<String, dynamic>)
          : null,
      tokens: json['tokens'] is Map<String, dynamic>
          ? TokenPair.fromJson(json['tokens'] as Map<String, dynamic>)
          : null,
    );
  }
}

class AuthSession {
  const AuthSession({required this.tokens, required this.user});

  final TokenPair tokens;
  final SafeUser user;

  Map<String, dynamic> toJson() => {
        'tokens': tokens.toJson(),
        'user': {
          'id': user.id,
          'username': user.username,
          'email': user.email,
          'role': user.role,
          'is_active': user.isActive,
          'is_verified': user.isVerified,
        },
      };

  factory AuthSession.fromJson(Map<String, dynamic> json) {
    return AuthSession(
      tokens: TokenPair.fromJson(
        (json['tokens'] ?? <String, dynamic>{}) as Map<String, dynamic>,
      ),
      user: SafeUser.fromJson(
        (json['user'] ?? <String, dynamic>{}) as Map<String, dynamic>,
      ),
    );
  }
}
