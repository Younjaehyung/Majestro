# arena_env_ma.py
import numpy as np


class MultiAgentArenaEnv:
    """
    2D 사각형 아레나 환경 (Multi-Agent)

    - arena 범위: [-map_range, map_range]^2
    - 플레이어 n_players, 몬스터 N(랜덤: monsters_min~monsters_max)
    - 액션(연속 3차원):
        a0: 이동 방향(각도) [-1,1] -> [-pi, pi]
        a1: 이동 강도(속도) [-1,1] -> [0, max_speed]
        a2: 공격 트리거 [-1,1] -> >0 이면 공격 시도
    - 관측:
        base(9) = (pos2, vel2, hp_norm, alive, team_alive_ratio2, time_frac)
        + 가까운 적 K명 (rel_pos2, hp_norm) => K*3
        + 가까운 아군 K명 (rel_pos2, hp_norm) => K*3

    [추가/수정 요약]
      1) 플레이어 원거리 공격(히트스캔/자동 타겟) 추가
      2) 맵 경계 밖으로 "나가려는 시도"에 패널티 추가
      3) viewer/train 코드 호환을 위해:
         - reset() -> (obs, info) 반환
         - k_nearest 인자 지원(k_near로 매핑)
         - obs_dim/act_dim/hp_*_max 속성 제공
      4) viewer에서 원거리 샷을 그릴 수 있도록 last_shots 이벤트 리스트 제공
         - step마다 초기화되며, 원거리 공격 시 {"start","end","hit","shooter","target","kind"}를 push
    """

    def __init__(
        self,
        seed=1,
        n_players=3,
        monsters_min=10,
        monsters_max=20,
        map_range=50.0,
        max_speed=0.25,
        # collision
        radius_player=0.15,
        radius_monster=0.12,
        # combat (근접 - 기존)
        attack_range=0.9,
        attack_damage=10.0,
        attack_cooldown_steps=6,
        # player ranged combat (추가)
        ranged_attack_range_player=3.5,
        ranged_attack_damage_player=6.0,
        ranged_attack_cooldown_steps_player=10,
        ranged_attack_requires_los=True,
        # reward
        time_penalty=0.01,
        penalty_out_of_bounds=0.2,  # (추가) 맵 경계를 넘어가려는 시도 패널티
        reward_damage=0.02,         # 준 데미지에 비례 보상
        penalty_damage=0.02,        # 받은 데미지에 비례 패널티(현재는 미적용, 주석 참고)
        reward_kill=4.0,
        reward_win=15.0,
        reward_lose=-15.0,
        # obs
        k_near=3,
        k_nearest=None,  # (추가) 구버전 코드 호환: k_nearest 인자 지원
        max_steps=300,
        # obstacles
        obstacles=None,
    ):
        self.rng = np.random.RandomState(seed)

        self.n_players = int(n_players)
        self.monsters_min = int(monsters_min)
        self.monsters_max = int(monsters_max)
        self.map_range = float(map_range)
        self.max_speed = float(max_speed)

        self.radius_player = float(radius_player)
        self.radius_monster = float(radius_monster)

        # 근접(기존)
        self.attack_range = float(attack_range)
        self.attack_damage = float(attack_damage)
        self.attack_cd = int(attack_cooldown_steps)

        # --- (추가) 플레이어 원거리 공격 파라미터 ---
        self.ranged_attack_range_p = float(ranged_attack_range_player)
        self.ranged_attack_damage_p = float(ranged_attack_damage_player)
        self.ranged_attack_cd_p = int(ranged_attack_cooldown_steps_player)
        self.ranged_attack_requires_los = bool(ranged_attack_requires_los)

        self.time_penalty = float(time_penalty)
        self.penalty_out_of_bounds = float(penalty_out_of_bounds)  # (추가)
        self.reward_damage = float(reward_damage)
        self.penalty_damage = float(penalty_damage)
        self.reward_kill = float(reward_kill)
        self.reward_win = float(reward_win)
        self.reward_lose = float(reward_lose)

        # --- (추가) 구버전 호환: k_nearest -> k_near ---
        if k_nearest is not None:
            k_near = k_nearest
        self.k_near = int(k_near)

        self.max_steps = int(max_steps) if max_steps is not None else None

        # obstacles: list of (x, y, radius)
        self.obstacles = obstacles if obstacles is not None else [
            (0.0, 0.0, 0.7),
            (1.8, -1.0, 0.6),
            (-1.5, 1.4, 0.55),
        ]

        # -----------------------------
        # (추가) viewer/train 호환용 속성
        # -----------------------------
        self.act_dim = 3
        # base(9) + enemy(K*3) + ally(K*3)
        self.obs_dim = 9 + 6 * self.k_near
        self.hp_player_max = 100.0
        self.hp_monster_max = 40.0

        # (추가) step 단위 발사 이벤트(뷰어 트레이서용)
        self.last_shots = []

        # state
        self.steps = 0
        self.monsters_n = 0

        # arrays
        self.p_pos = None
        self.p_vel = None
        self.p_hp = None
        self.p_cd = None

        self.m_pos = None
        self.m_vel = None
        self.m_hp = None
        self.m_cd = None

        self.reset()

    def _agent_ids(self):
        ids = [f"P{i}" for i in range(self.n_players)]
        ids += [f"M{i}" for i in range(self.monsters_n)]
        return ids

    def reset(self):
        self.steps = 0
        self.monsters_n = int(self.rng.randint(self.monsters_min, self.monsters_max + 1))

        # (추가) 발사 이벤트 초기화
        self.last_shots = []

        # players
        self.p_pos = np.zeros((self.n_players, 2), dtype=np.float32)
        self.p_vel = np.zeros((self.n_players, 2), dtype=np.float32)
        self.p_hp = np.ones((self.n_players,), dtype=np.float32) * self.hp_player_max
        self.p_cd = np.zeros((self.n_players,), dtype=np.int32)

        # monsters
        self.m_pos = np.zeros((self.monsters_n, 2), dtype=np.float32)
        self.m_vel = np.zeros((self.monsters_n, 2), dtype=np.float32)
        self.m_hp = np.ones((self.monsters_n,), dtype=np.float32) * self.hp_monster_max
        self.m_cd = np.zeros((self.monsters_n,), dtype=np.int32)

        left_x = -0.6 * self.map_range
        right_x = 0.6 * self.map_range

        # 플레이어는 왼쪽에 세로로 배치
        ys = np.linspace(-0.3 * self.map_range, 0.3 * self.map_range, self.n_players)
        for i in range(self.n_players):
            self.p_pos[i] = np.array([left_x, ys[i]], dtype=np.float32)

        # 몬스터는 오른쪽에 y 랜덤 배치
        for i in range(self.monsters_n):
            y = self.rng.uniform(-0.8 * self.map_range, 0.8 * self.map_range)
            self.m_pos[i] = np.array([right_x, y], dtype=np.float32)

        # (수정) 구버전 코드 호환: (obs, info) 반환
        return self._get_obs(), {}

    def step(self, action_dict):
        self.steps += 1

        # (추가) 이번 step의 원거리 발사 이벤트 초기화(뷰어 트레이서용)
        self.last_shots = []

        # 기본 보상: 시간 패널티
        rew = {aid: -self.time_penalty for aid in self._agent_ids()}

        # cd 감소
        self.p_cd = np.maximum(self.p_cd - 1, 0)
        self.m_cd = np.maximum(self.m_cd - 1, 0)

        # 1) 이동 (수정: 경계 이탈 시도 패널티를 rew에 반영)
        self._apply_moves(action_dict, rew)

        # 2) 공격(데미지/킬 보상 포함)
        self._apply_attacks(action_dict, rew)

        # 종료 판정
        done = False
        trunc = False

        players_alive = int(np.sum(self.p_hp > 0))
        monsters_alive = int(np.sum(self.m_hp > 0))

        if monsters_alive <= 0:
            done = True
            # 플레이어 승
            for i in range(self.n_players):
                rew[f"P{i}"] += self.reward_win
            for i in range(self.monsters_n):
                rew[f"M{i}"] += self.reward_lose
        elif players_alive <= 0:
            done = True
            # 몬스터 승
            for i in range(self.n_players):
                rew[f"P{i}"] += self.reward_lose
            for i in range(self.monsters_n):
                rew[f"M{i}"] += self.reward_win

        if self.max_steps is not None and self.steps >= self.max_steps and not done:
            trunc = True

        obs = self._get_obs()
        info = {aid: {} for aid in self._agent_ids()}
        term = {aid: done for aid in self._agent_ids()}
        truncs = {aid: trunc for aid in self._agent_ids()}

        return obs, rew, term, truncs, info

    # -----------------------
    # Observation
    # -----------------------
    def _get_obs(self):
        obs = {}
        # 전역 정보
        p_alive = float(np.sum(self.p_hp > 0)) / float(self.n_players)
        m_alive = float(np.sum(self.m_hp > 0)) / float(max(1, self.monsters_n))
        time_frac = float(self.steps) / float(max(1, self.max_steps if self.max_steps else 1))

        for i in range(self.n_players):
            obs[f"P{i}"] = self._build_obs_for_player(i, p_alive, m_alive, time_frac)
        for i in range(self.monsters_n):
            obs[f"M{i}"] = self._build_obs_for_monster(i, p_alive, m_alive, time_frac)
        return obs

    def _build_obs_for_player(self, idx, p_alive, m_alive, time_frac):
        # 자기 상태
        me_pos = self.p_pos[idx]
        me_vel = self.p_vel[idx]
        me_hp = self.p_hp[idx]
        me_alive = 1.0 if me_hp > 0 else 0.0

        # 적/아군 nearest K
        enemy_feats = self._nearest_entities(me_pos, self.m_pos, self.m_hp, self.k_near)
        ally_feats = self._nearest_entities(me_pos, self.p_pos, self.p_hp, self.k_near, exclude_idx=idx)

        base = np.array([
            me_pos[0], me_pos[1],
            me_vel[0], me_vel[1],
            me_hp / self.hp_player_max,
            me_alive,
            p_alive, m_alive, time_frac
        ], dtype=np.float32)

        return np.concatenate([base, enemy_feats, ally_feats], axis=0)

    def _build_obs_for_monster(self, idx, p_alive, m_alive, time_frac):
        me_pos = self.m_pos[idx]
        me_vel = self.m_vel[idx]
        me_hp = self.m_hp[idx]
        me_alive = 1.0 if me_hp > 0 else 0.0

        enemy_feats = self._nearest_entities(me_pos, self.p_pos, self.p_hp, self.k_near)
        ally_feats = self._nearest_entities(me_pos, self.m_pos, self.m_hp, self.k_near, exclude_idx=idx)

        base = np.array([
            me_pos[0], me_pos[1],
            me_vel[0], me_vel[1],
            me_hp / self.hp_monster_max,
            me_alive,
            m_alive, p_alive, time_frac  # monster perspective
        ], dtype=np.float32)

        return np.concatenate([base, enemy_feats, ally_feats], axis=0)

    def _nearest_entities(self, me_pos, all_pos, all_hp, k, exclude_idx=None):
        # 살아있는 것만
        alive = all_hp > 0
        if exclude_idx is not None:
            alive = alive.copy()
            alive[exclude_idx] = False

        idxs = np.nonzero(alive)[0]
        if len(idxs) == 0:
            return np.zeros((k * 3,), dtype=np.float32)  # rel_x, rel_y, hp

        pos = all_pos[idxs]
        hp = all_hp[idxs]

        d = np.linalg.norm(pos - me_pos[None, :], axis=1)
        order = np.argsort(d)[:k]

        out = []
        for j in order:
            rel = pos[j] - me_pos
            # hp 정규화: 플레이어/몬스터 모두 100 스케일을 쓰던 기존 코드 호환을 위해 100으로 나눔
            out.extend([float(rel[0]), float(rel[1]), float(hp[j]) / 100.0])
        # pad
        while len(out) < k * 3:
            out.extend([0.0, 0.0, 0.0])

        return np.asarray(out, dtype=np.float32)

    # -----------------------
    # Action decode
    # -----------------------
    def _decode_move(self, a):
        a0 = float(a[0])
        a1 = float(a[1])

        theta = np.pi * np.clip(a0, -1.0, 1.0)
        sp = (np.clip(a1, -1.0, 1.0) + 1.0) * 0.5 * self.max_speed

        d = np.array([np.cos(theta), np.sin(theta)], dtype=np.float32) * sp
        return d.astype(np.float32)

    # -----------------------
    # Movement & collision
    # -----------------------
    def _apply_moves(self, action_dict, rew_dict):
        # 플레이어
        for i in range(self.n_players):
            if self.p_hp[i] <= 0:
                self.p_vel[i] = 0
                continue

            a = np.asarray(action_dict.get(f"P{i}", np.zeros(3, dtype=np.float32)), dtype=np.float32)
            d = self._decode_move(a)

            # --- (추가) 맵 경계를 넘어가려는 '시도'에 패널티 부여 ---
            # 충돌/클램프 적용 전 의도한 위치(desired_pos)가 유효 범위를 벗어나면 패널티를 준다.
            desired_pos = self.p_pos[i] + d
            min_x = -self.map_range + self.radius_player
            max_x = self.map_range - self.radius_player
            min_y = -self.map_range + self.radius_player
            max_y = self.map_range - self.radius_player

            overflow_x = 0.0
            overflow_y = 0.0
            if desired_pos[0] < min_x:
                overflow_x = float(min_x - desired_pos[0])
            elif desired_pos[0] > max_x:
                overflow_x = float(desired_pos[0] - max_x)

            if desired_pos[1] < min_y:
                overflow_y = float(min_y - desired_pos[1])
            elif desired_pos[1] > max_y:
                overflow_y = float(desired_pos[1] - max_y)

            overflow = float(np.sqrt(overflow_x * overflow_x + overflow_y * overflow_y))
            if overflow > 0.0:
                # max_speed로 정규화해서 '조금 삐져나가려는 시도'는 약하게, '세게 박는 시도'는 더 강하게 패널티
                rew_dict[f"P{i}"] -= self.penalty_out_of_bounds * (overflow / (self.max_speed + 1e-6))

            new_pos, new_vel = self._move_with_collision(self.p_pos[i], d, self.radius_player)
            self.p_vel[i] = new_vel
            self.p_pos[i] = new_pos

        # 몬스터
        for i in range(self.monsters_n):
            if self.m_hp[i] <= 0:
                self.m_vel[i] = 0
                continue

            a = np.asarray(action_dict.get(f"M{i}", np.zeros(3, dtype=np.float32)), dtype=np.float32)
            d = self._decode_move(a)

            # --- (추가) 몬스터도 동일하게 맵 경계 이탈 시도 패널티 ---
            desired_pos = self.m_pos[i] + d
            min_x = -self.map_range + self.radius_monster
            max_x = self.map_range - self.radius_monster
            min_y = -self.map_range + self.radius_monster
            max_y = self.map_range - self.radius_monster

            overflow_x = 0.0
            overflow_y = 0.0
            if desired_pos[0] < min_x:
                overflow_x = float(min_x - desired_pos[0])
            elif desired_pos[0] > max_x:
                overflow_x = float(desired_pos[0] - max_x)

            if desired_pos[1] < min_y:
                overflow_y = float(min_y - desired_pos[1])
            elif desired_pos[1] > max_y:
                overflow_y = float(desired_pos[1] - max_y)

            overflow = float(np.sqrt(overflow_x * overflow_x + overflow_y * overflow_y))
            if overflow > 0.0:
                rew_dict[f"M{i}"] -= self.penalty_out_of_bounds * (overflow / (self.max_speed + 1e-6))

            new_pos, new_vel = self._move_with_collision(self.m_pos[i], d, self.radius_monster)
            self.m_vel[i] = new_vel
            self.m_pos[i] = new_pos

    def _move_with_collision(self, pos, delta, radius):
        # 1) 경계 충돌(사각형)
        p = pos + delta
        p[0] = np.clip(p[0], -self.map_range + radius, self.map_range - radius)
        p[1] = np.clip(p[1], -self.map_range + radius, self.map_range - radius)

        # 2) 장애물 충돌(원형) - 간단하게 밀어내기
        for (ox, oy, orad) in self.obstacles:
            c = np.array([ox, oy], dtype=np.float32)
            v = p - c
            dist = float(np.linalg.norm(v) + 1e-8)
            min_d = float(orad + radius)
            if dist < min_d:
                # obstacle 밖으로 push
                n = v / dist
                p = c + n * min_d

        vel = (p - pos).astype(np.float32)
        return p.astype(np.float32), vel

    # -----------------------
    # Combat helpers
    # -----------------------
    def _has_line_of_sight(self, p0, p1):
        """p0->p1 선분이 원형 장애물에 의해 막히는지 검사.
        - True: 시야 확보(막히지 않음)
        - False: 장애물이 가로막음
        """
        if len(self.obstacles) == 0:
            return True

        p0 = np.asarray(p0, dtype=np.float32)
        p1 = np.asarray(p1, dtype=np.float32)
        d = p1 - p0
        denom = float(np.dot(d, d)) + 1e-8

        for (ox, oy, orad) in self.obstacles:
            c = np.array([ox, oy], dtype=np.float32)

            # p0->p1 선분에서 원 중심 c까지의 최소 거리 계산(closest point)
            t = float(np.dot(c - p0, d) / denom)
            t = float(np.clip(t, 0.0, 1.0))
            closest = p0 + d * t

            dist = float(np.linalg.norm(closest - c))
            if dist < float(orad):
                return False

        return True

    def _segment_first_circle_hit(self, p0, p1):
        """(추가) p0->p1 선분이 장애물 원과 교차하면, 가장 가까운 교차점 반환.
        교차가 없으면 None 반환.
        - viewer에서 LOS가 막혔을 때 '장애물에 맞는 지점'으로 tracer 끝점을 찍기 위해 사용.
        """
        if len(self.obstacles) == 0:
            return None

        p0 = np.asarray(p0, dtype=np.float32)
        p1 = np.asarray(p1, dtype=np.float32)
        d = p1 - p0
        a = float(np.dot(d, d))
        if a < 1e-8:
            return None

        best_t = None
        best_pt = None

        for (ox, oy, orad) in self.obstacles:
            c = np.array([ox, oy], dtype=np.float32)
            f = p0 - c

            b = 2.0 * float(np.dot(f, d))
            cc = float(np.dot(f, f)) - float(orad) * float(orad)

            disc = b * b - 4.0 * a * cc
            if disc < 0.0:
                continue

            sqrt_disc = float(np.sqrt(disc))
            t1 = (-b - sqrt_disc) / (2.0 * a)
            t2 = (-b + sqrt_disc) / (2.0 * a)

            # 선분 내부에서 가장 작은 t를 선택
            for t in (t1, t2):
                if 0.0 <= t <= 1.0:
                    if best_t is None or t < best_t:
                        best_t = t
                        best_pt = p0 + d * t

        if best_pt is None:
            return None
        return best_pt.astype(np.float32)

    # -----------------------
    # Combat
    # -----------------------
    def _apply_attacks(self, action_dict, rew_dict):
        # 플레이어 공격 -> 몬스터 피격
        for pi in range(self.n_players):
            if self.p_hp[pi] <= 0:
                continue
            if self.p_cd[pi] > 0:
                continue

            a = np.asarray(action_dict.get(f"P{pi}", np.zeros(3, dtype=np.float32)), dtype=np.float32)
            if a[2] <= 0.0:
                continue

            # 가장 가까운 살아있는 몬스터 찾기
            alive = self.m_hp > 0
            if not np.any(alive):
                continue

            epos = self.m_pos[alive]
            idx_map = np.nonzero(alive)[0]
            dists = np.linalg.norm(epos - self.p_pos[pi][None, :], axis=1)
            j = int(np.argmin(dists))
            dist = float(dists[j])
            tid = int(idx_map[j])

            did_attack = False

            # 2-A) 근접 공격
            if dist <= (self.attack_range + self.radius_monster):
                killed, dmg = self._try_attack(
                    attacker_pos=self.p_pos[pi],
                    target_pos=self.m_pos,
                    target_hp=self.m_hp,
                    target_radius=self.radius_monster,
                    damage=self.attack_damage,
                    range_=self.attack_range,
                )
                if dmg > 0.0:
                    rew_dict[f"P{pi}"] += self.reward_damage * float(dmg)
                if killed:
                    rew_dict[f"P{pi}"] += self.reward_kill

                self.p_cd[pi] = self.attack_cd
                did_attack = True

            # 2-B) 원거리 공격(추가): 히트스캔 + (옵션) LOS
            elif dist <= (self.ranged_attack_range_p + self.radius_monster):
                p0 = self.p_pos[pi].copy()
                p1 = self.m_pos[tid].copy()

                hit = True
                if self.ranged_attack_requires_los and (not self._has_line_of_sight(p0, p1)):
                    hit = False

                # (추가) viewer tracer를 위해 이벤트 기록
                end_pt = p1
                if not hit:
                    # 장애물에 막혔으면 '첫 교차점'을 tracer 끝점으로 사용(없으면 target까지)
                    hit_pt = self._segment_first_circle_hit(p0, p1)
                    if hit_pt is not None:
                        end_pt = hit_pt

                self.last_shots.append({
                    "kind": "ranged",              # (추가)
                    "shooter": f"P{pi}",          # (추가)
                    "target": f"M{tid}",          # (추가)
                    "start": p0,                  # (추가)
                    "end": end_pt.copy(),         # (추가)
                    "hit": bool(hit),             # (추가)
                })

                if hit:
                    before = float(self.m_hp[tid])
                    self.m_hp[tid] = max(0.0, self.m_hp[tid] - float(self.ranged_attack_damage_p))
                    after = float(self.m_hp[tid])

                    dealt = before - after
                    killed = (after <= 0.0)

                    if dealt > 0.0:
                        rew_dict[f"P{pi}"] += self.reward_damage * float(dealt)
                    if killed:
                        rew_dict[f"P{pi}"] += self.reward_kill

                # LOS에 막혀도 "발사 시도"는 했으므로 쿨다운 소비
                self.p_cd[pi] = self.ranged_attack_cd_p
                did_attack = True

            if not did_attack:
                pass

        # 몬스터 공격 -> 플레이어 피격 (근접만)
        for mi in range(self.monsters_n):
            if self.m_hp[mi] <= 0:
                continue
            if self.m_cd[mi] > 0:
                continue

            a = np.asarray(action_dict.get(f"M{mi}", np.zeros(3, dtype=np.float32)), dtype=np.float32)
            if a[2] <= 0.0:
                continue

            killed, dmg = self._try_attack(
                attacker_pos=self.m_pos[mi],
                target_pos=self.p_pos,
                target_hp=self.p_hp,
                target_radius=self.radius_player,
                damage=self.attack_damage,
                range_=self.attack_range,
            )
            if dmg > 0.0:
                rew_dict[f"M{mi}"] += self.reward_damage * float(dmg)
            if killed:
                rew_dict[f"M{mi}"] += self.reward_kill

            self.m_cd[mi] = self.attack_cd

        # NOTE:
        # penalty_damage(피격 패널티)는 "누가 맞았는지"를 추적하거나,
        # step 시작 시 hp 스냅샷을 저장했다가 hp diff로 계산해야 정확히 적용 가능.
        # 현재 버전은 구현 편의상 생략(기존 코드와 동일한 한계).

    def _try_attack(self, attacker_pos, target_pos, target_hp, target_radius, damage, range_):
        alive = target_hp > 0
        if not np.any(alive):
            return False, 0.0

        epos = target_pos[alive]
        idx_map = np.nonzero(alive)[0]

        d = np.linalg.norm(epos - attacker_pos[None, :], axis=1)
        j = int(np.argmin(d))
        if float(d[j]) <= float(range_) + float(target_radius):
            tid = int(idx_map[j])
            before = float(target_hp[tid])
            target_hp[tid] = max(0.0, float(target_hp[tid]) - float(damage))
            after = float(target_hp[tid])
            dealt = before - after
            killed = (after <= 0.0)
            return killed, float(dealt)
        return False, 0.0
